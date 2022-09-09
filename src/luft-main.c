#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

struct lua_ver_list {
  char item[6][10];
  int len;
};

struct lua_ver_name {
  char name[10];   /* Ex: 5.4 */
  char altname[10];/* Ex: 54 (common in non-Linux */
};

struct path_list {
  char item[100][PATH_MAX];
  int  len;
};


/* Convert the environment PATH in a list of directories */
static int get_path_list(struct path_list *pl) {
  int ret=0;
  int len=0;
  char *envpath = getenv("PATH");
  char *path = malloc(sizeof(char) * strlen(envpath)+1);
  char *item = malloc(sizeof(char) * PATH_MAX);;
  if (envpath == NULL) {
    ret = 1;
    goto end;
  }

  strcpy(path, envpath);
  for (; ;path=NULL) if (len < 100) {
    item = strtok(path,":");
    if (item == NULL) break;
    strcpy(pl->item[len], item);
    len++;
  }
  pl->len = len;

  end:
    free(path); free(item);
    return ret;
}

static char const *lv_exp =
  "lua%*[{ =]%[0-9.]%*[, ]%[0-9.]%*[, ]%[0-9.]%*[, ]%[0-9.]%*[, ]%[0-9.]%*[, ]%[0-9.]";



/* Read file line by line until find the Lua required version */
static struct lua_ver_list required_lua(char *filename) {
  FILE  *fp = fopen(filename, "r");
  char  *line  = NULL;
  size_t linelen = 0;
  int    vv = 0;
  struct lua_ver_list V;
  V.len = 0;

  if (fp) {
    while (getline(&line, &linelen, fp) != -1) {
      vv = sscanf(line, lv_exp,
          V.item[0],
          V.item[1],
          V.item[2],
          V.item[3],
          V.item[4],
          V.item[5]);
      if (vv) {
        V.len = vv;
        break;
      }
    }
    free(line);
    fclose(fp);
  }

  return V;
}



/* Checks if version is valid example "5.1", ... "5.4" */
static void check_lua_ver_name(char *ver, struct lua_ver_name *lvn) {
  char altver[100] = "\0"; /* if ver="5.4" this will be "54" */
  int  altverc     = 0;

  for(int i=0; i<strlen(ver); i++) {
    if (ver[i] >= '0' && ver[i] <= '9') {
      altver[altverc++] = ver[i];
      continue;
    }

    if (ver[i] != '.') {
      fprintf(stderr, "Invalid Lua version representation: %s", ver);
      exit(1);
    }
  }
  altver[altverc]='\0';

  strcpy(lvn->name, ver);
  strcpy(lvn->altname, altver);
}



/* Find Lua executable containing version allusion in name */
static int find_lua_bin (char *fn, char *dir, struct lua_ver_name *v) {

  if (v == NULL) {
    sprintf(fn,"%s/%s",dir,"lua");
    if (access(fn, X_OK) == 0) return 0;
  } else {
    sprintf(fn,"%s/%s%s",dir,"lua",v->name);
    if (access(fn, X_OK) == 0) return 0;

    sprintf(fn,"%s/%s%s",dir,"lua",v->altname);
    if (access(fn, X_OK) == 0) return 0;
  }
  return 1;
}



static int match_version(char *file, char* ver) {
  char cmd[PATH_MAX];
  char bv[10];
  FILE *p;

  sprintf(cmd,"%s -e 'print(_VERSION)'", file);
  p = popen(cmd, "r");
  if (p != NULL && fscanf(p, "%*s %[0-9.]", bv) > 0 && strcmp(ver, bv) == 0) {
      return 0;
  }
  return 1;
}



static void call_lua(char *binfile, char *luftindex) {
  printf("\nCalling Lua:\n%s %s", binfile, luftindex);
  exit(1);
}



/* Process the shebanged file */
static int luft_load_folder(char *luftindex) {
  char   realfile[PATH_MAX];    /* luftindex realpath */
  char   bin[PATH_MAX];         /* the Lua executable */
  struct lua_ver_list versions;
  struct lua_ver_name lvn;
  struct path_list path;

  if (realpath(luftindex, realfile) == NULL) {
    printf("%s", strerror(errno));
    return 1;
  }

  versions = required_lua(realfile);
  get_path_list(&path);

  for (int vi=0; vi < versions.len; vi++) {
    check_lua_ver_name(versions.item[vi], &lvn);
    for (int pi=0; pi < path.len; pi++) {
      if ( find_lua_bin(bin, path.item[pi], &lvn) == 0) {
        call_lua(bin, realfile);
      }
    }
  }

  for (int pi=0; pi < path.len; pi++) {
    if ( find_lua_bin(bin, path.item[pi], NULL) == 0) {
      for (int vi=0; vi < versions.len; vi++) {
        if (match_version(bin, versions.item[vi]) == 0) {
          call_lua(bin, realfile);
        }
      }
    }
  }

  return 0;
}

static int ispath(char *str) {
  for (int i=0; str[i] != '\0'; i++) {
    if (str[i] == '/') return 1;
  }
  return 0;
}


int main(int argc, char **argv) {
  printf("\nArgs %d", argc);
  if (argc > 1 && ispath(argv[1]) ) {
      luft_load_folder(argv[1]);
  }
  printf("\niuiuiuiuiui");
  return 0;
}

