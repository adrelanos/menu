/* Copyright:  GPL */
/*
  Written by joost witteveen;
  read_pkginfo function by Tom Lees, run_menumethods by both.

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
#include <config.h>
#include "update-menus.h"

using std::set;
using std::vector;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;

static const char * home_dir;

int debug=0, verbose=0;
set<string> installed_packages;
set<string> menufiles_processed;

extern char **environ;

translateinfo *transinfo;
configinfo     config;

DIR *open_dir_check(string dirname)
{
  struct stat st;

  if (stat(dirname.c_str(), &st) || (!S_ISDIR (st.st_mode)))
    throw dir_error_read(dirname);

  return opendir (dirname.c_str());
}

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

bool is_pkg_installed(string filename)
{
  if (contains(filename, "local.",0))
    return true;
  else
    return installed_packages.find(filename) != installed_packages.end();
}

menuentry::menuentry(parsestream &i, const string &filename)
{
  char c = i.get_char();
  if (c == '?') {
    read_menuentry(i);
  } else {
    // old format menuentry
    if (!is_pkg_installed(filename)) {
      i.skip_line();
      throw cond_inst_false();
    }
    data[PACKAGE_VAR] = filename;
    i.put_back(c);
    data[NEEDS_VAR]   = i.get_stringconst();
    data[SECTION_VAR] = i.get_stringconst();
    i.get_stringconst(); //id is unused
    data[ICON_VAR]    = i.get_stringconst();
    data[TITLE_VAR]   = i.get_stringconst();
    i.skip_space();
    data[COMMAND_VAR] = i.get_line();
  }
}

bool menuentry::check_validity(parsestream &i, string &name)
{
  string function = i.get_name();
  if (function != COND_PACKAGE)
    throw unknown_cond_package(&i, function);
  i.skip_char('(');
  name = i.get_name();
  if (!is_pkg_installed(name)) {
    i.skip_line();
    throw cond_inst_false();
  }
  i.skip_char(')');
  return true;
}

void menuentry::read_menuentry(parsestream &i)
{
  // a new format menuentry
  string name;

  // read available info
  check_validity(i, name);
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

void menuentry::output(vector<string> &s)
{
  string t;
  std::map<string, string>::const_iterator i = data.begin();
  if (i != data.end()) {
    while (true)
    {
      t += i->first + "=\"" + i->second + "\"";
      i++;
      if (i == data.end())
          break;
      else
          t += ' ';
    }
  }
  t += '\n';
  config.report(string("ADDING: ")+t, configinfo::report_debug);
  s.push_back(t);
}

ostream &menuentry::debugoutput(ostream &o)
{
  std::map<string, string>::const_iterator i;

  o << "MENUENTRY:" << endl;
  for (i = data.begin(); i != data.end(); ++i)
      o << "  data[" << i->first << "]=" << i->second << endl;
  return o;
}

/////////////////////////////////////////////////////
//  configinfo
//

void configinfo::parse_def(const string &key, const string& value)
{
  if (key=="compat") {
    if (value=="menu-1") compat = parsestream::eol_newline;
    else if(value=="menu-2") compat = parsestream::eol_semicolon;
  } else if(key=="verbosity") {
    if     (value=="quiet")   verbosity=report_quiet;
    else if(value=="normal")  verbosity=report_normal;
    else if(value=="verbose") verbosity=report_verbose;
    else if(value=="debug")   verbosity=report_debug;
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

void configinfo::update(string filename)
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
    parse_def(str.substr(0, pos), str.substr(pos + 1));
  }

}

void configinfo::report(const string &message, verbosity_type v)
{
  if(v <= verbosity) {
    switch(method) {
    case method_stdout:
      cout << "update-menus[" << getpid() << "]: " << message << endl;
      break;
    case method_stderr:
      cerr << "update-menus[" << getpid() << "]: " << message << endl;
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

bool trans_class::check(string &s)
{
  config.report(string("checking ")+match+" < "+s,configinfo::report_debug);
  return contains(match, s, 0);
}

string trans_class::debuginfo()
{
  return string(" match=")+match+", replace="+replace+", replace_var="+replace_var;
}

void translate::process(menuentry &m, const string &v)
{
  if(v==match)
      m.data[replace_var]=replace;
}

void subtranslate::process(menuentry &m, const string &v)
{
  if(contains(v, match, 0))
      m.data[replace_var]=replace;
}

void substitute::process(menuentry &m, const string &v){
  string s,*t;
  if(contains(v, match, 0)) {
    t=&(m.data[replace_var]);
    if(t->length()>=replace.length())
      *t=replace+t->substr(match.length());
  }
}

void translateinfo::init(parsestream &i)
{
  Regex ident("[a-zA-Z_][a-zA-Z0-9_]*");

  config.report(string("Reading translate info in ")+i.filename(),
      configinfo::report_verbose);
  while (true)
  {
    string name = i.get_name(ident);
    config.report(string("name=")+name, configinfo::report_debug);
    i.skip_space();
    string match_var = i.get_name(ident);
    config.report(string("match_var=")+match_var, configinfo::report_debug);
    i.skip_space();
    i.skip_char('-');
    i.skip_char('>');
    i.skip_space();
    string replace_var = i.get_name(ident);
    config.report(string("replace_var=")+replace_var, configinfo::report_debug);

    i.skip_line();
    while (true)
    {
      trans_class *trcl;

      i.skip_space();
      string match = i.get_stringconst();
      if (match == ENDTRANSLATE_TRANS) {
        i.skip_line();
        break;
      }
      if (match[0]=='#') {
        i.skip_line();
        continue;
      }
      i.skip_space();
      string replace = i.get_stringconst();
      if (name == TRANSLATE_TRANS)
          trcl = new translate(match,replace,replace_var);
      if (name == SUBTRANSLATE_TRANS)
          trcl = new subtranslate(match,replace,replace_var);
      if (name == SUBSTITUTE_TRANS)
          trcl = new substitute(match,replace,replace_var);

      std::pair<const string, trans_class *> p(match,trcl);

      config.report(string("adding translate rule: [")+p.first+
          "]"+ trcl->debuginfo(),
          configinfo::report_debug);
      trans[match_var].insert(p);
      i.skip_line();
    }
  }
}

translateinfo::translateinfo(parsestream &i)
{
  init(i);
}

translateinfo::translateinfo(const string &filename)
{
  try {
    config.report(string("Attempting to open ") + filename + ".. ",
        configinfo::report_debug);
    parsestream ps(filename);

    init(ps);
  }
  catch(endoffile p) {
    config.report("End reading translate info", configinfo::report_debug);
  }

}

void translateinfo::process(menuentry &m)
{
  std::map<string, trans_map>::const_iterator i;
  trans_map::const_iterator j;
  string *match;
  for(i=trans.begin(); i!=trans.end(); i++){
    match=&m.data[(*i).first];
    j=(*i).second.lower_bound(*match);
    if((j==(*i).second.end()) ||
       ((j!=(*i).second.begin()) && ((*j).first != *match)))
      j--;
    do {
      config.report(string("translate: var[")+*match+"]"+
          " testing trans rule match for:"+
          j->first,
          configinfo::report_debug);
      j->second->process(m,*match);
      j++;
    } while((j != i->second.end()) && j->second->check(*match));

  }
}

void translateinfo::debuginfo()
{
  std::map<string, trans_map>::const_iterator i;
  trans_map::const_iterator j;
  for(i = trans.begin(); i != trans.end(); ++i)
  {
    config.report(string("TRANS: [")+(*i).first+"]", configinfo::report_debug);
    for (j = i->second.begin(); j != i->second.end(); ++j)
        config.report(string("key=")+(*j).first+(*j).second->debuginfo()+'\n',
            configinfo::report_debug);
  }
}
/////////////////////////////////////////////////////
//  Installed Package Status:
//

void read_pkginfo()
{
  // Here we get the list of *installed* packages from dpkg, using sed to
  // retrieve the package name.
  char *pkgs = "dpkg-query --show --showformat='${status} ${package}\\n' | sed -n -e 's/.*installed *//p'";
  FILE *status = popen(pkgs, "r");

  if(!status)
    throw pipeerror_read(pkgs);

  config.report(string("Reading installed packages..."),
                  configinfo::report_verbose);

  while (!feof(status))
  {
    char tmp[MAX_LINE];
    fgets(tmp, MAX_LINE, status);

    if(tmp[strlen(tmp)-1]=='\n')
        tmp[strlen(tmp)-1]=0;

    installed_packages.insert(tmp);
  }
  pclose(status);
}

void read_menufile(const string &filename, const string &shortfilename,
                   vector<string> &menudata)
{
  parsestream *ps = 0;
  std::ifstream *pipe_istr = 0;

  config.report(string("Reading menuentryfile ") + filename, configinfo::report_debug);
  try {
    if (executable(filename)) {
      pipe_istr = new std::ifstream(filename.c_str(), std::ios::in);
      try {
        ps = new parsestream(*pipe_istr);
      } catch (endoffile d) {
        cerr<<"Error (or no input available from stdout) while executing "<<endl
            <<filename<<" . Note that it is"<<endl
            <<"a _feature_ of menu that it executes menuentryfiles that have"<<endl
            <<"the executable bit set. See the documentation."<<endl;
        throw endoffile(d);
      }
    } else {
      ps = new parsestream(filename);
    }
  } catch (endoffile p) {
    return;
  }

  try {
    ps->seteolmode(config.compat);

    bool wrote_filename = false;
    int linenr = 1;
    while (ps)
    {
      try {
        menuentry m(*ps, shortfilename);
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
  } catch(endoffile p) {
      if (executable(filename)) {
          delete pipe_istr;
      }
    delete ps;
  }
}

void read_menufilesdir(vector<string> &menudata)
{
  for(vector<string>::const_iterator method_i = config.menufilesdir.begin();
      method_i != config.menufilesdir.end();
      ++method_i)
  {
    int count = menudata.size();
    string dirname = *method_i;
    config.report(string("Reading menuentryfiles in ")+dirname,
        configinfo::report_verbose);
    try {
      struct dirent *entry;
      DIR *dir = open_dir_check(dirname);
      while((entry = readdir(dir)))
      {
        string name = entry->d_name;
        if((name != "README") && (name != "core") && (name[0] != '.') &&
            (name.find(".bak") == string::npos) &&
            (!contains(name, "menu.config")) &&
            (name[name.length()-1] != '~'))
            if(menufiles_processed.find(name) == menufiles_processed.end()) {
              menufiles_processed.insert(name);
              name = dirname+name;
              struct stat st;
              int r = stat(name.c_str(),&st);
              try {
                if((!r)&&(S_ISREG(st.st_mode)||S_ISLNK(st.st_mode)))
                    read_menufile(name,entry->d_name, menudata);
              }
              catch (endofline p) {
                cerr << "Error reading " << name << endl;
              }
            }
      }
    } catch (dir_error_read p) { }
    config.report(itostring(menudata.size() - count) + string(" menu entries found (") + itostring(menudata.size()) + string(" total)"), configinfo::report_verbose);
  }
}

void run_menumethod(string methodname, const vector<string> &menudata)
{
  int fds[2];
  const char *args[] = { methodname.c_str(), NULL };
  pid_t child, r;
  int status;

  config.report(string("Running method: ")+methodname, configinfo::report_verbose);

  if (pipe(fds) == -1) {
      config.report(string("Cannot create pipe"), configinfo::report_quiet);
      exit(1);
  }

  if(!(child=fork())) {
    //child:
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
    execve(args[0],(char **)args, environ);
    exit(1);
  } else {
    //parent:
    signal(SIGPIPE,SIG_IGN);
    close(fds[0]);

    for(vector<string>::const_iterator i = menudata.begin(); i != menudata.end(); ++i)
        write(fds[1], i->c_str(), i->length());

    close(fds[1]);
    r = wait4(child, &status, 0, NULL);
    signal(SIGPIPE,SIG_DFL);
  }
  if (r == -1)
    config.report(string("Script ")+methodname+" could not be executed.",
        configinfo::report_quiet);
  if (WEXITSTATUS(status))
    config.report(string("Script ")+methodname+" returned error status "
        +itostring(WEXITSTATUS(status))+".",
        configinfo::report_quiet);
  else if (WIFSIGNALED(status))
    config.report(string("Script ")+methodname+" recieved signal "
        +itostring(WTERMSIG(status))+".",
        configinfo::report_quiet);
}

void run_menumethoddir (const string &dirname, const vector<string> &menudata)
{
  struct stat st;
  DIR *dir;
  struct dirent *entry;
  char *s, tmp[MAX_LINE];
  int r;

  config.report(string("Running menu-methods in ")+dirname, configinfo::report_verbose);
  dir=open_dir_check(dirname);
  while ((entry = readdir (dir)) != NULL) {
    if (!strcmp(entry->d_name, "README") || !strcmp(entry->d_name, "core"))
      continue;
    for (s = entry->d_name; *s != '\0'; s++){
      if (!(isalnum (*s) || (*s == '_') || (*s == '-')))
          break;
    }
    if (*s != '\0')
      continue;

    sprintf (tmp, "%s%s", dirname.c_str(), entry->d_name);
    r=stat (tmp, &st);

    // Do we have execute permissions? 
    if ((!r) &&
        (((st.st_mode & S_IXOTH) || 
          ((st.st_mode & S_IXGRP) && st.st_gid == getegid ()) || 
          ((st.st_mode & S_IXUSR) && st.st_uid == geteuid ())) && 
         (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))))
        run_menumethod(tmp, menudata);
  }
  closedir (dir);
}

int create_lock()
{
  // return lock fd if succesful, false if unsuccesfull.
  int fd=true;
  char buf[64];

  if (!getuid()) {
    fd = open(UPMEN_LOCKFILE,O_WRONLY|O_CREAT,00644);

    if(flock(fd,LOCK_EX|LOCK_NB)) {
      if(errno==EWOULDBLOCK) {
        config.report(string("Other update-menus processes are already "
              "locking " UPMEN_LOCKFILE ", quitting."),
            configinfo::report_verbose);
      }else{
        config.report(string("Cannot lock "UPMEN_LOCKFILE": ")+
            strerror(errno)+ " Aborting.",
            configinfo::report_quiet);	
      }
      return false;

    }

    sprintf(buf, "%d", getpid());
    if (write(fd, buf, sizeof(buf) < 1)) {
      config.report("Cannot write to lockfile "UPMEN_LOCKFILE". Aborting.",
          configinfo::report_quiet);
      return false;
    }
  }
  return fd;
}

void remove_lock()
{
  if (!getuid()){
    if (unlink(UPMEN_LOCKFILE))
      config.report("Cannot remove lockfile "UPMEN_LOCKFILE,
          configinfo::report_normal);
  }
}

int check_dpkglock()
{
  //return 1 if DPKG_LOCKFILE is locked and we should wait
  //return 0 if we don't need to wait (not root, or no dpkg lock)
  int fd;
  struct flock fl;
  char buf[MAX_LINE];
  int er;
  if(getuid()){
    config.report("update-menus run by user -- cannot determine if dpkg is "
        "locking "DPKG_LOCKFILE": assuming there is no lock",
        configinfo::report_verbose);

    return 0;
  }
  fd=open(DPKG_LOCKFILE, O_RDWR|O_CREAT|O_TRUNC, 0660);
  if(fd==-1)
    return 0; // used to be 1, but why??? (Should not happen, anyway)
  fl.l_type= F_WRLCK;
  fl.l_whence= SEEK_SET;
  fl.l_start= 0;
  fl.l_len= 0;
  if (fcntl(fd,F_SETLK,&fl) == -1) {
    er=errno;
    close(fd);
    if (er == EWOULDBLOCK || er == EAGAIN || er == EACCES)
        return 1;
    cerr<<"update-menus: Encountered an unknown errno (="
        <<er<<")."<<endl
        <<"update-menus: Could you please be so kind as to email joostje@debian.org"<<endl
        <<"update-menus: the errno (="<<er<<") with a discription of what you did to"<<endl
        <<"update-menus: trigger this. Thanks very much."<<endl
        <<"Press enter"<<endl;
    std::cin.get(buf, sizeof(buf));
    return 1;
  }
  fl.l_type= F_UNLCK;
  fl.l_whence= SEEK_SET;
  fl.l_start= 0;
  fl.l_len= 0;
  if (fcntl(fd,F_SETLK,&fl) == -1){
    // `cannot happen'
    cerr<<"update-menus: ?? Just locked the dpkg status database to see if another dpkg"<<endl
        <<"update-menus: Was running. Now I cannot unlock it! Aborting"<<endl;
    exit(1);
  }
  close(fd);
  return 0;
}

void exit_on_signal(int signr)
{
  exit(0);
}

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
  // and only after the parent receved the signal it wil exit(0);

  if(check_dpkglock()){
    sigset_t sig,oldsig;

    // Apparently libc2 on 2.0 kernels, with threading on, blocks
    // SIGUSR1. This blocking would be inhereted by children, so
    // as apt was compiled with -lpthread, this caused problems in 
    // update-menus. I now get rid of that by using
    //  - SIGUSR2 instead of SIGUSR1,
    //  - sigprocmask to unblock SIGUSR2.
    // Eighter one of those solutions should be enough, though.

    sigemptyset(&sig);
    sigaddset(&sig,SIGUSR2);
    sigprocmask(SIG_UNBLOCK,&sig,&oldsig);

    signal(SIGUSR2,exit_on_signal);
    parentpid=getpid();
    if((child=fork())){
      if(child==-1){
        perror("update-menus: fork");
        exit(1);
      }
      pause();
    } else {
      r=create_lock();
      if(r){
        stdoutfile=string("/tmp/update-menus.")+itostring(getpid());
        config.report("waiting for dpkg to finish (forking to background)\n"
            "(checking " DPKG_LOCKFILE ")",
            configinfo::report_normal);
        config.report(string("further output (if any) will appear in ")
            +stdoutfile,
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
    r=create_lock();
    if(!r){
      exit(1);
    }
    config.report("Dpkg not locking dpkg status area. Good.",
        configinfo::report_verbose);
  }
}

void parse_params(char **argv)
{
  while (*(++argv)) {
    if(string("-d") == *argv)
      config.set_verbosity(configinfo::report_debug);
    if(string("-v") == *argv)
      config.set_verbosity(configinfo::report_verbose);
    if(string("--nodefaultdirs") == *argv)
      config.usedefaultmenufilesdirs = false;
    if(string("--stdout") == *argv)
      config.onlyoutput_to_stdout = true;
    if(string("--menufiledir") == *argv) {
      argv++;
      if(*argv) {
          config.menufilesdir.push_back(*argv);
      } else {
        cerr<<_("directory expected after --menufilesdir option")<<endl;
        throw informed_fatal();
      }
    }
    if(string("--menumethod") == *argv) {
      argv++;
      if(*argv) {
          config.menumethod = *argv;
      } else {
        cerr<<_("filename expected after --menumethod option")<<endl;
        throw informed_fatal();
      }
    }
    if(string("-h") == *argv || string("--help") == *argv) {
      cerr <<
          _("update-menus: update the various window-manager config files (and\n"
              "  dwww, and pdmenu) Usage: update-menus [options] \n"
              "    -v  Be verbose about what is going on\n"
              "    -d  Debugging (loads of unintelligible output)\n"
              "    -h, --help This message\n"
              "    --menufiledir <dir> Add <dir> to the lists of menu directories to search\n"
              "    --menumethod  <method> Run only the menu method <method>\n"
              "    --nodefaultdirs Disables the use of all the standard menu directories\n"
              "    --stdout Output menu list in format suitable for piping to install-menu\n");
      exit(1);
    }
  }
}

void read_userconfiginfo()
{
  if (getuid()) {
    try {
      config.update(string(home_dir)+"/"+USERCONFIG);
    } catch(ferror_open d) { };
  }
}

void read_rootconfiginfo()
{
  if(!transinfo){
    try {
      config.update(CONFIG_FILE);
    } catch (ferror_open d){};
  }
}

void read_usertranslateinfo()
{
  if (getuid()) {
    try {
      transinfo = new translateinfo(string(home_dir)+"/"+USERTRANSLATE);
    } catch(ferror_open d) { };
  }
}

void read_roottranslateinfo()
{
  if (!transinfo) {
    try {
      transinfo = new translateinfo(TRANSLATE_FILE);
    } catch (ferror_open d) { };
  }
}

void read_homedirectory()
{
  struct passwd *pwentry = getpwuid(getuid());

  if (pwentry != NULL) {
    home_dir = pwentry->pw_dir;
  } else {
    home_dir = getenv("HOME");
  }
}

int main (int argc, char **argv)
{
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
    if (transinfo)
        transinfo->debuginfo();
    if (config.usedefaultmenufilesdirs) {
      if (getuid())
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
      if (getuid()) {
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
