/*
 * Debian menu system -- update-menus
 * update-menus/update-menus.cc
 *
 * Copyright (C) 1996-2003  Joost Witteveen, 
 * Copyright (C) 2002-2004  Bill Allombert and Morten Brix Pedersen.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License with
 * the Debian GNU/Linux distribution in file /usr/share/common-licenses/GPL;
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Written by Joost Witteveen;
 * read_pkginfo function by Tom Lees, run_menumethods by both.
 */

#include <fstream>
#include <sstream>
#include <set>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <cctype>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <dirent.h>
#include <signal.h>
#include <syslog.h>
#include <pwd.h>
#include "config.h"
#include "update-menus.h"
#include "stringtoolbox.h"

using std::set;
using std::vector;
using std::string;
using std::cout;
using std::cerr;
using std::ostream;
using namespace exceptions;

static const char * home_dir;

set<string> installed_packages;
set<string> menufiles_processed;

translateinfo *transinfo;
configinfo     config;
bool is_root;

/** Try to open a directory. Throws dir_error_read if failed, and a DIR*
 * descriptor if succeeded.
 */
DIR *open_dir_check(const string& dirname)
{
  struct stat st;

  if (stat(dirname.c_str(), &st) || (!S_ISDIR (st.st_mode)))
      throw dir_error_read();

  return opendir(dirname.c_str());
}

/** Checks whether a file is executable */
bool executable(const string &s)
{
  struct stat st;
  if (stat(s.c_str(), &st)) {
    return false;
  } else {
    return (((st.st_mode & S_IXOTH) ||
          ((st.st_mode & S_IXGRP) && st.st_gid == getegid ()) ||
          ((st.st_mode & S_IXUSR) && st.st_uid == geteuid ())) &&
        (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)));
  }
}

/** Checks whether a package is installed */
bool is_pkg_installed(const string& filename)
{
  if (contains(filename, "local."))
    return true;
  else
    return installed_packages.find(filename) != installed_packages.end();
}

menuentry::menuentry(parsestream &i, const std::string& file, const std::string& shortfile)
{
  char c = i.get_char();
  if (c == '?') {
    read_menuentry(i);
  } else {
    // old format menuentry
    if (!is_pkg_installed(shortfile)) {
      i.skip_line();
      throw cond_inst_false();
    }
    data[PACKAGE_VAR] = shortfile;
    i.put_back(c);
    data[NEEDS_VAR]   = i.get_stringconst();
    data[SECTION_VAR] = i.get_stringconst();
    i.get_stringconst(); //id is unused
    data[ICON_VAR]    = i.get_stringconst();
    data[TITLE_VAR]   = i.get_stringconst();
    i.skip_space();
    data[COMMAND_VAR] = i.get_line();
  }
  check_req_tags(file);
}

/** This function checks the package name to see if it's valid and installed.
 * Multiple package names can exist, seperated by comma. */
void menuentry::check_pkg_validity(parsestream &i, std::string &name)
{
  string function = i.get_name();
  if (function != COND_PACKAGE)
    throw unknown_cond_package(&i, function);

  // Read an entry, such as (foo) or (foo, bar)
  char c;
  while ((c = i.get_char()))
  {
      if (c == ',' || c == '(') {
          // A package name follows.
          string pkgname = i.get_name();

          if (!name.empty())
                name += ", ";

          name += pkgname;
          if (!is_pkg_installed(pkgname)) {
              i.skip_line();
              throw cond_inst_false();
          }
      } else if (c == ')') {
          // We are finished with the package requirements.
          return;
      }
  }
  i.skip_char(')');
}

/** Checks whether we have all the tags we need. */
void menuentry::check_req_tags(const std::string& filename)
{
  vector<string> need;

  need.push_back(SECTION_VAR);
  need.push_back(TITLE_VAR);
  need.push_back(NEEDS_VAR);

  for(vector<string>::iterator i = need.begin(); i != need.end(); ++i)
  {
    std::map<string, string>::const_iterator j = data.find(*i);
    if ((j == data.end()) || j->second.empty())
        throw missing_tag(filename, *i);
  }

}

/** Parse a menuentry from a parsestream */
void menuentry::read_menuentry(parsestream &i)
{
  // a new format menuentry
  string name;

  // read available info
  check_pkg_validity(i, name);
  data[PACKAGE_VAR] = name;
  i.skip_space();
  i.skip_char(':');
  i.doescaping = false;

  // read menuentry:
  try {
    char c;
    do {
      string key = i.get_name();
      string value = i.get_eq_stringconst();
      data[key] = value;
      c = i.get_char();
      i.put_back(c);
    } while(c);
  }
  catch (endofline) { }
  i.skip_line();
}

void menuentry::output(std::vector<std::string> &s)
{
  string t;
  std::map<string, string>::const_iterator i = data.begin();
  if (i != data.end()) {
    while (true)
    {
      t += i->first + "=\"" + i->second + '"';
      i++;
      if (i == data.end())
          break;
      else
          t += ' ';
    }
  }
  t += '\n';
  s.push_back(t);
}

/////////////////////////////////////////////////////
//  configinfo
//

/** Handle key/value configuration pair */
void configinfo::parse_config(const std::string &key, const std::string& value)
{
  if (key=="compat") {
    if (value=="menu-1") compat = parsestream::eol_newline;
    else if(value=="menu-2") compat = parsestream::eol_semicolon;
  } else if(key=="verbosity") {
    if     (value=="quiet")   verbosity=report_quiet;
    else if(value=="normal")  verbosity=report_normal;
    else if(value=="verbose") verbosity=report_verbose;
    else if(value=="debug") verbosity=report_debug;
  } else if(key=="method") {
    if     (value=="stdout")  method=method_stdout;
    else if(value=="stderr")  method=method_stderr;
    else if(value=="syslog") {

      method = method_syslog;
      string facility;
      string priority;

      std::istringstream ss(value);
      ss >> facility;
      ss >> priority;

           if(facility=="auth")       syslog_facility=LOG_AUTH;
      else if(facility=="authpriv")   syslog_facility=LOG_AUTHPRIV;
      else if(facility=="authcron")   syslog_facility=LOG_CRON;
      else if(facility=="authdaemon") syslog_facility=LOG_AUTHPRIV;
      else if(facility=="authkern")   syslog_facility=LOG_KERN;
      else if(facility=="authlocal0") syslog_facility=LOG_LOCAL0;
      else if(facility=="authlocal1") syslog_facility=LOG_LOCAL1;
      else if(facility=="authlocal2") syslog_facility=LOG_LOCAL2;
      else if(facility=="authlocal3") syslog_facility=LOG_LOCAL3;
      else if(facility=="authlocal4") syslog_facility=LOG_LOCAL4;
      else if(facility=="authlocal5") syslog_facility=LOG_LOCAL5;
      else if(facility=="authlocal6") syslog_facility=LOG_LOCAL6;
      else if(facility=="authlocal7") syslog_facility=LOG_LOCAL7;
      else if(facility=="authlpr")    syslog_facility=LOG_LPR;
      else if(facility=="authmail")   syslog_facility=LOG_MAIL;
      else if(facility=="authnews")   syslog_facility=LOG_NEWS;
      else if(facility=="authsyslog") syslog_facility=LOG_SYSLOG;
      else if(facility=="authuser")   syslog_facility=LOG_USER;
      else if(facility=="authuucp")   syslog_facility=LOG_UUCP;

           if(priority=="emerg")      syslog_priority=LOG_EMERG;
      else if(priority=="alert")      syslog_priority=LOG_ALERT;
      else if(priority=="crit")       syslog_priority=LOG_CRIT;
      else if(priority=="err")        syslog_priority=LOG_ERR;
      else if(priority=="warning")    syslog_priority=LOG_WARNING;
      else if(priority=="notice")     syslog_priority=LOG_NOTICE;
      else if(priority=="info")       syslog_priority=LOG_INFO;
      else if(priority=="debug")      syslog_priority=LOG_DEBUG;
    }
  }
}

void configinfo::read_file(const std::string& filename)
{
  std::ifstream config_file(filename.c_str());

  if (!config_file)
      return;

  string str;
  while (getline(config_file, str))
  {
    // read a key and a value, seperated by '='.
    string::size_type pos = str.find("=");
    if (pos == string::npos)
        return;

    // parse key and value
    parse_config(str.substr(0, pos), str.substr(pos + 1));
  }

}

void configinfo::report(const std::string &message, verbosity_type v)
{
  if(v <= verbosity) {
    switch(method) {
    case method_stdout:
      cout << String::compose("update-menus[%1]: %2\n",getpid(),message);
      break;
    case method_stderr:
      cerr << String::compose("update-menus[%1]: %2\n",getpid(),message);
      break;
    case method_syslog:
      openlog("update-menus",LOG_PID,syslog_facility);
      syslog(syslog_priority,message.c_str());
      closelog();
    }
  }
}

/////////////////////////////////////////////////////
//  translate stuff
//

void translate::process(menuentry &m, const std::string &search)
{
  if (search == match)
      m.data[replace_var] = replace;
}

void subtranslate::process(menuentry &m, const std::string &search)
{
  if (contains(search, match))
      m.data[replace_var] = replace;
}

void substitute::process(menuentry &m, const std::string &search)
{
  if (contains(search, match)) {
    string *current = &(m.data[replace_var]);
    if (current->length() >= match.length())
        *current = replace + current->substr(match.length());
  }
}

translateinfo::translateinfo(const std::string &filename)
{
  parsestream *i = 0;
  try {
    i = new parsestream(filename);

    Regex ident("[a-zA-Z_][a-zA-Z0-9_]*");
    /*Translation here and below refer to the file 
      /etc/menu-methods/translate_menus that allow to rename and reorganize
      menu entries automatically. It does not refer to the localisation
      (translation to other languages).
     */
    config.report(String::compose(_("Reading translation rules in %1."), i->filename()),
        configinfo::report_verbose);
    while (true)
    {
      string name = i->get_name(ident);
      i->skip_space();
      string match_var = i->get_name(ident);
      i->skip_space();
      i->skip_char('-');
      i->skip_char('>');
      i->skip_space();
      string replace_var = i->get_name(ident);

      i->skip_line();
      while (true)
      {
        i->skip_space();
        string match = i->get_stringconst();
        if (match == ENDTRANSLATE_TRANS) {
          i->skip_line();
          break;
        }
        if (match[0]=='#') {
          i->skip_line();
          continue;
        }
        i->skip_space();
        string replace = i->get_stringconst();
        trans_class *trcl;
        if (name == TRANSLATE_TRANS) {
          trcl = new translate(match,replace,replace_var);
        } else if (name == SUBTRANSLATE_TRANS) {
          trcl = new subtranslate(match,replace,replace_var);
        } else if (name == SUBSTITUTE_TRANS) {
          trcl = new substitute(match,replace,replace_var);
        } else {
          i->skip_line();
          continue;
        }

        std::pair<const string, trans_class *> p(match, trcl);

        trans[match_var].push_back(p);
        i->skip_line();
      }
    }
  } catch(endoffile p) { }
  delete i;
}

void translateinfo::process(menuentry &m)
{
  std::map<string, std::vector<trans_pair> >::const_iterator i;
  std::vector<trans_pair>::const_iterator j;
  string *search;
  for (i = trans.begin(); i != trans.end(); ++i)
  {
    search = &m.data[i->first];
    for (j = i->second.begin(); j != i->second.end(); ++j)
    {
      j->second->process(m, *search);
    }
  }
}

/////////////////////////////////////////////////////
//  Installed Package Status:
//

/** Read in list of installed packages */
void read_pkginfo()
{
  // Here we get the list of *installed* packages from dpkg, using sed to
  // retrieve the package name.
  char *pkgs = "dpkg-query --show --showformat='${status} ${package}\\n' | sed -n -e 's/.*installed *//p'";
  FILE *status = popen(pkgs, "r");

  if (!status)
    throw pipeerror_read(pkgs);

  config.report(_("Reading installed packages list..."),
                  configinfo::report_verbose);

  while (!feof(status))
  {
    char tmp[MAX_LINE];
    if (fgets(tmp, MAX_LINE, status) != NULL) {
      if (tmp[strlen(tmp)-1] == '\n')
          tmp[strlen(tmp)-1] = 0;

      installed_packages.insert(tmp);
    }
  }
  pclose(status);
}

/** Read a menufile and create a menuentry for it */
void read_menufile(const string &filename, const string &shortfilename,
                   vector<string> &menudata)
{

  parsestream *ps = 0;
  std::stringstream *sstream = 0;

  try {
    // Whenever we encounter a file which has the executable bit set, we
    // need to execute it and read its output from stdout.

    if (executable(filename)) {
      FILE *status = popen(filename.c_str(), "r");

      sstream = new std::stringstream;

      if (!status)
          throw pipeerror_read(filename.c_str());

      while (!feof(status))
      {
        char tmp[MAX_LINE];
        if (fgets(tmp, MAX_LINE, status) != NULL)
            *sstream << tmp;
      }
      pclose(status);

      try {
        ps = new parsestream(*sstream);
      } catch (endoffile d) {
        cerr << String::compose(_("Execution of %1 generated no output or returned an error.\n"), filename);
        throw endoffile(d);
      }
    } else {
      ps = new parsestream(filename);
    }
  } catch (endoffile p) {
    delete sstream;
    return;
  }

  try {
    ps->seteolmode(config.compat);

    bool wrote_filename = false;
    int linenr = 1;
    while (true)
    {
      try {
        menuentry m(*ps, filename, shortfilename);
        linenr++;
        if (transinfo)
            transinfo->process(m);
        //gettext_translate(m);
        if (!wrote_filename) {
          menudata.push_back(string("!F ") + filename + '\n');
          wrote_filename = true;
        }
        m.output(menudata);
        if (ps->linenumber() != linenr) {
          menudata.push_back(string("!L ") + itostring(ps->linenumber())+ '\n');
          linenr = ps->linenumber();
        }
      }
      catch (cond_inst_false) { }
    }
  } 
  catch (endoffile p) { }
  catch (missing_tag& exc) {
    std::cerr << exc.message() << std::endl;
    std::cerr << _("Skipping file because of errors...\n");
  }
  catch (except_pi& exc) {
    exc.report();
    std::cerr << _("Skipping file because of errors...\n");
  }

  delete ps;
  delete sstream;
}

/** Read a directory full of menu files */
void read_menufilesdir(vector<string> &menudata)
{
  for(vector<string>::const_iterator method_i = config.menufilesdir.begin();
      method_i != config.menufilesdir.end();
      ++method_i)
  {
    int count = menudata.size();
    string dirname = *method_i;
    config.report(String::compose(_("Reading menu-entry files in %1."), dirname),
        configinfo::report_verbose);
    try {
      struct dirent *entry;
      DIR *dir = open_dir_check(dirname);
      while((entry = readdir(dir)))
      {
        string name = entry->d_name;
        if ((name != "README") && (name != "core") && (name[0] != '.') &&
            (name.find(".bak") == string::npos) &&
            (!contains(name, "menu.config")) &&
            (name[name.length()-1] != '~'))

            if (menufiles_processed.find(name) == menufiles_processed.end()) {
              menufiles_processed.insert(name);
              name = dirname+name;
              struct stat st;
              int r = stat(name.c_str(),&st);
              try {
                if ((!r) && (S_ISREG(st.st_mode)||S_ISLNK(st.st_mode)))
                    read_menufile(name,entry->d_name, menudata);
              }
              catch (endofline p) {
                cerr << String::compose(_("Error reading %1.\n"), name);
              }
            }
      }
    } catch (dir_error_read p) { }
    config.report(String::compose(_("%1 menu entries found (%2 total)."), menudata.size() - count, menudata.size()), configinfo::report_verbose);
  }
}

/** Run a menu method */
void run_menumethod(string methodname, const vector<string> &menudata)
{
  int fds[2];
  const char *args[] = { methodname.c_str(), NULL };
  pid_t child, r;
  int status;

  config.report(String::compose(_("Running method: %1"), methodname), configinfo::report_verbose);

  if (pipe(fds) == -1) {
      config.report(_("Cannot create pipe."), configinfo::report_quiet);
      exit(1);
  }

  if (!(child=fork())) {
    // child:
    close(fds[1]);
    close(0);
    dup(fds[0]);

    //???
    // The next 2 lines seem strange! But if I leave it out,
    // pipes (in commands executed in the /etc/menu-method/* scripts as
    // postrun etc) don't work when ran from dpkg. (When run from dpkg,
    // we elsewhere close all filedescriptors. And apparently
    // we need STDOUT open somehow in order to make the pipes work.
    // (changed by suggestion of Fabian Sturm <f@rtfs.org>, for GNU Hurd,
    // bug #105674

    close(1);
    open("/dev/null", O_RDWR);
    execv(args[0],(char **)args);
    exit(1);
  } else {
    // parent:
    signal(SIGPIPE,SIG_IGN);
    close(fds[0]);

    for(vector<string>::const_iterator i = menudata.begin(); i != menudata.end(); ++i)
        write(fds[1], i->c_str(), i->length());

    close(fds[1]);
    r = wait4(child, &status, 0, NULL);
    signal(SIGPIPE,SIG_DFL);
  }
  if (r == -1)
    config.report(String::compose(_("Script %1 could not be executed."), methodname),
        configinfo::report_quiet);
  if (WEXITSTATUS(status))
    config.report(String::compose(_("Script %1 returned error status %2."), methodname, WEXITSTATUS(status)),
        configinfo::report_quiet);
  else if (WIFSIGNALED(status))
    config.report(String::compose(_("Script %1 received signal %2."), methodname, WTERMSIG(status)),
        configinfo::report_quiet);
}

/** Run a directory full of menu methods */
void run_menumethoddir(const string &dirname, const vector<string> &menudata)
{
  struct dirent *entry;
  char *s;

  config.report(String::compose(_("Running menu-methods in %1."), dirname), configinfo::report_verbose);
  DIR *dir = open_dir_check(dirname);
  while ((entry = readdir (dir)) != NULL) {
    if (!strcmp(entry->d_name, "README") || !strcmp(entry->d_name, "core"))
      continue;
    for (s = entry->d_name; *s != '\0'; s++)
    {
      if (!(isalnum (*s) || (*s == '_') || (*s == '-')))
          break;
    }
    if (*s != '\0')
      continue;
    
    string method = dirname + entry->d_name;

    if (executable(method))
        run_menumethod(method, menudata);
  }
  closedir (dir);
}

/** Try to create a lock file for update-menus */
int create_lock()
{
  // return lock fd if succesful, false if unsuccesfull.
  int fd = true;
  char buf[64];

  if (is_root) {
    fd = open(UPMEN_LOCKFILE,O_WRONLY|O_CREAT,00644);

    if (flock(fd,LOCK_EX|LOCK_NB)) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        config.report(String::compose(_("Other update-menus processes are already locking %1, quitting."), UPMEN_LOCKFILE),
            configinfo::report_verbose);
      } else {
        config.report(String::compose(_("Cannot lock %1: %2 - Aborting."), UPMEN_LOCKFILE, strerror(errno)),
            configinfo::report_quiet);	
      }
      return false;

    }

    sprintf(buf, "%d", getpid());
    if (write(fd, buf, sizeof(buf) < 1)) {
      config.report(String::compose(_("Cannot write to lockfile %1 - Aborting."), UPMEN_LOCKFILE),
          configinfo::report_quiet);
      return false;
    }
  }
  return fd;
}

/** Try to remove update-menus lock */
void remove_lock()
{
  if (is_root) {
    if (unlink(UPMEN_LOCKFILE))
      config.report(String::compose(_("Cannot remove lockfile %1."), UPMEN_LOCKFILE),
          configinfo::report_normal);
  }
}

/** Check whether dpkg is locked
  * return 1 if DPKG_LOCKFILE is locked and we should wait
  * return 0 if we don't need to wait (not root, or no dpkg lock)
  * when in doubt return 0 to avoid deadlocks.
  */
int check_dpkglock()
{
  int fd;
  struct flock fl;
  if (!is_root)
  {
    config.report(_("Update-menus is run by user."), configinfo::report_verbose);
    return 0;
  }
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;
  fd = open(DPKG_LOCKFILE, O_RDWR|O_CREAT|O_TRUNC, 0660);
  if (fd == -1)
    /* Probably /var/lib/dpkg does not exist.
     * Most probably dpkg is not running.
     */
      return 0; 
  if (fcntl(fd,F_GETLK,&fl) == -1)
  {
    /* Probably /var/lib/dpkg filesystem does not support
     * locks.
     */
    close(fd);
    return 0;
  }
  close(fd);
  return fl.l_type!=F_UNLCK;
}

void exit_on_signal(int signr)
{
  exit(0);
}

/** Check whether dpkg is running, and fork into background waiting if it is
 */
void wait_dpkg(string &stdoutfile)
{
  int child;
  int parentpid;
  int i,r;

  // This function checks to see if dpkg is running, and if
  // so, forks into the background, to let dpkg go on installing
  // packages. After dpkg finished (and wrote the new packages file),
  // this function wakes up, and returns.
  
  // Writing the console output correctly is actually non-trivial.
  // The problem is that we in the following if statement
  //     if(child=fork())
  //       exit(0);                            //parent process
  //     else
  //       do something (and write to stdout); //child `background' process
  // want to write to stdout. But if we write to stdout there,
  // the exit(0) may already have occured, and dpkg may already
  // have started writing stuff to the console. To prevent that,
  // I use signals: the `background' process sents a signal to the
  // parent once it's written everything it wants to stdout,
  // and only after the parent received the signal it will exit(0);
  // [Oh god! (added by Bill trying to understand the fork() business)]

  if (check_dpkglock()) {
    sigset_t sig,oldsig;

    // Apparently libc2 on 2.0 kernels, with threading on, blocks
    // SIGUSR1. This blocking would be inherited by children, so
    // as apt was compiled with -lpthread, this caused problems in 
    // update-menus. I now get rid of that by using
    //  - SIGUSR2 instead of SIGUSR1,
    //  - sigprocmask to unblock SIGUSR2.
    // Either one of those solutions should be enough, though.

    sigemptyset(&sig);
    sigaddset(&sig,SIGUSR2);
    sigprocmask(SIG_UNBLOCK,&sig,&oldsig);

    signal(SIGUSR2, exit_on_signal);
    parentpid=getpid();
    if ((child=fork())) {
      if (child==-1) {
        perror("update-menus: fork");
        exit(1);
      }
      pause();
    } else {
      r = create_lock();
      if (r) {
        stdoutfile = string("/tmp/update-menus.")+itostring(getpid());
        config.report(String::compose(_("Waiting for dpkg to finish (forking to background).\n"
            "(checking %1)"), DPKG_LOCKFILE),
            configinfo::report_normal);
        config.report(String::compose(_("Further output (if any) will appear in %1."), stdoutfile),
            configinfo::report_normal);
        // Close all fd's except the lock fd, for daemon mode.
        for (i=0;i<32;i++) {
          if (i != r)
              close(i);
        }
        kill(parentpid, SIGUSR2);

        while(check_dpkglock())
            sleep(2);
      } else {
        // Exit without doing anything. Kill parent too!
        kill(parentpid,SIGUSR2);
        exit(0);
      }
    }
  } else {
    r = create_lock();
    if (!r)
        exit(1);

    config.report(_("Dpkg is not locking dpkg status area, good."),
        configinfo::report_verbose);
  }
}

/** Print usage information */
void usage(ostream &c)
{
      c <<
          /* This is the update-menus --help message*/
          _("update-menus: update the various window-manager config files (and\n"
              "  dwww, and pdmenu) Usage: update-menus [options] \n"
              "    -d              Output debugging messages.\n"
              "    -v              Be verbose about what is going on.\n"
              "    -h, --help      This message.\n"
              "    --menufilesdir  <dir> Add <dir> to the lists of menu directories to search.\n"
              "    --menumethod    <method> Run only the menu method <method>.\n"
              "    --nodefaultdirs Disables the use of all the standard menu directories.\n"
              "    --stdout        Output menu list in format suitable for piping to\n"
              "                    install-menu.\n")
          /* This is the end of the update-menus --help message*/
          << _(  "    --version       Output version information and exit.\n"  );
}

/** Parse commandline parameters */
void parse_params(char **argv)
{
  while (*(++argv))
  {
    if (string("-v") == *argv) {
      config.set_verbosity(configinfo::report_verbose);
      continue;
    } else if (string("-d") == *argv) {
      config.set_verbosity(configinfo::report_debug);
      continue;
    } else if (string("--nodefaultdirs") == *argv) {
      config.usedefaultmenufilesdirs = false;
      continue;
    } else if (string("--stdout") == *argv) {
      config.onlyoutput_to_stdout = true;
      continue;
    } else if (string("--menufilesdir") == *argv || string("--menufiledir") == *argv) {
      argv++;
      if(*argv) {
          config.menufilesdir.push_back(*argv);
      } else {
        cerr<< _("Directory is expected after --menufilesdir option.\n");
        throw informed_fatal();
      }
      continue;
    } else if (string("--menumethod") == *argv) {
      argv++;
      if(*argv) {
          config.menumethod = *argv;
      } else {
        cerr << _("Filename is expected after --menumethod option.\n");
        throw informed_fatal();
      }
      continue;
    } else if (string("--version") == *argv) {
        cout << "update-menus "VERSION << std::endl;
        exit(0);
    } else if (string("-h") == *argv || string("--help") == *argv) {  
      usage(cout);
      exit(0);
    } else {
      usage(cerr);
      exit(1);
    }
  }
}

/** Read users configuration file */
void read_userconfiginfo()
{
  if (!is_root) {
    try {
      config.read_file(string(home_dir)+"/"+USERCONFIG);
    } catch(ferror_open d) { };
  }
}

/** Read roots configuration file */
void read_rootconfiginfo()
{
  if(!transinfo){
    try {
      config.read_file(CONFIG_FILE);
    } catch (ferror_open d){};
  }
}

/** Read users translate information */
void read_usertranslateinfo()
{
  if (!is_root) {
    try {
      transinfo = new translateinfo(string(home_dir)+"/"+USERTRANSLATE);
    } catch(ferror_open d) { };
  }
}

/** Read roots translate information */
void read_roottranslateinfo()
{
  if (!transinfo) {
    try {
      transinfo = new translateinfo(TRANSLATE_FILE);
    } catch (ferror_open d) { };
  }
}

/** Find our home directory */
void read_homedirectory()
{
  struct passwd *pwentry = getpwuid(getuid());

  if (pwentry != NULL)
      home_dir = pwentry->pw_dir;
  else
      home_dir = getenv("HOME");
}

int main (int argc, char **argv)
{
  is_root = (getuid() == 0);
  read_homedirectory();

  vector<string> menudata;
  string stdoutfile;
  struct stat st;

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  try {
    read_rootconfiginfo();
    parse_params(argv);
    wait_dpkg(stdoutfile);
    if(!stdoutfile.empty()) {
      close(1);
      open(stdoutfile.c_str(), O_WRONLY|O_CREAT|O_SYNC|O_EXCL, 0666);
      close(2);
      dup2(1,2);
    }
    read_pkginfo();
    transinfo = 0;

    read_usertranslateinfo();
    read_roottranslateinfo();

    if (config.usedefaultmenufilesdirs) {
      if (!is_root)
          config.menufilesdir.push_back(string(home_dir)+"/"+USERMENUS);
      config.menufilesdir.push_back(CONFIGMENUS);
      config.menufilesdir.push_back(PACKAGEMENUSLIB);
      config.menufilesdir.push_back(PACKAGEMENUS);
      config.menufilesdir.push_back(MENUMENUS);
    }

    read_menufilesdir(menudata);

    if (config.onlyoutput_to_stdout) {
        for(vector<string>::const_iterator i = menudata.begin(); i != menudata.end(); ++i)
              cout << *i;

    } else if (!config.menumethod.empty()) {
      run_menumethod(config.menumethod, menudata);
    } else {
      if (!is_root) {
        try {
          run_menumethoddir(string(home_dir)+"/"+USERMETHODS, menudata);
        }
        catch(dir_error_read d) {
          run_menumethoddir(MENUMETHODS, menudata);
        }
      } else {
          run_menumethoddir(MENUMETHODS, menudata);
      }
    }
  }
  catch(genexcept& p) { p.report(); }

  remove_lock();

  if(!stdoutfile.empty())
      if (!stat(stdoutfile.c_str(),&st))
          if (!st.st_size)
              unlink(stdoutfile.c_str());
}
