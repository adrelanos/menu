
#include <stdio.h>
#include <stdlib.h>
//#include <types.h>
#include <regex.h>

void *re_alloc_pattern_c(){
  return malloc(sizeof(struct re_pattern_buffer));
}

const char *re_compile_pattern_c (const char *REGEX, int REGEX_SIZE,
			    void *PATTERN_BUFFER){
  return re_compile_pattern (REGEX, REGEX_SIZE,
			     (struct re_pattern_buffer *)PATTERN_BUFFER);
}
