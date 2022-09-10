#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdarg.h>

#define DEBUG printf


struct {
  int len;
  struct {
    char name[10]; /* Ex: 5.4 */
    char alt[10];  /* Ex: 54 (common in non-Linux */
  } item[6];
} lua_ver;


struct {
  char item[100][PATH_MAX];
  int  len;
} path_list;

static char luft_script[PATH_MAX] = "\0";
static char luft_root[PATH_MAX]   = "\0";
static char luft_file[PATH_MAX]   = "\0";
static char ERROR[1024];

/* matches the line version */
static char const *lv_exp =
  "__VERSIONS = %*[\"{ ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]";


static void luft_error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args); fprintf(stderr,"\n");
  va_end(args);
  exit(1);
}


/* Convert the environment PATH in a list of directories */
static int get_path_list() {
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
    strcpy(path_list.item[len], item);
    len++;
  }
  path_list.len = len;

  end:
    free(path); free(item);
    return ret;
}



/* Checks if version is valid example "5.1", ... "5.4" and populate @lvn */
static void validate_lua_ver() {
  char alt[100] = "\0"; /* if ver="5.4" this will be "54" */
  int  ai       = 0;
  char *name;

  for(int v=0; v < lua_ver.len; v++) {
    name=lua_ver.item[v].name;
    for(int c=0; c<strlen(name); c++) {
      if (name[c] >= '0' && name[c] <= '9') {
        alt[ai++] = name[c];
        continue;
      }
      if (name[c] != '.') luft_error("Invalid version %s", name);
    }
    alt[ai]='\0';
    strcpy(lua_ver.item[v].alt,alt);
  }
}



/* Read file line by line until find the Lua required version */
static void required_lua() {
  FILE  *fp = fopen(luft_file, "r");
  char  *line  = NULL;
  size_t linelen = 0;
  int    linenum = 0;
  int    vv = 0;
  lua_ver.item[0].name[0] = '\0';
  lua_ver.len = 0;

  if (fp) {
    while (getline(&line, &linelen, fp) != -1 && linenum++ < 3) {
      vv = sscanf(line, lv_exp,
          lua_ver.item[0].name,
          lua_ver.item[1].name,
          lua_ver.item[2].name,
          lua_ver.item[3].name,
          lua_ver.item[4].name,
          lua_ver.item[5].name);
      if (vv) {
        lua_ver.len = vv;
        validate_lua_ver();
        break;
      }
    }
    free(line);
    fclose(fp);

    if (lua_ver.item[0].name[0] == '\0') {
      luft_error("no valid '__VERSIONS' found at the beginning of '%s'", luft_file);
    }
  }
}



/* Returns 0 when can successfully run the Lua binary and get _VERSION */
static int check_lua_ver(char *bin, char *ver) {
  char cmd[PATH_MAX];
  FILE *p;
  sprintf(cmd, "%s -e 'print(_VERSION)'", bin);
  p = popen(cmd,"r");
  if (p != NULL) {
    if (fscanf(p,"%*s %[0-9.]",ver) > 0) {
      pclose(p);
      return 0;
    }
    pclose(p);
  }
  ver = NULL;
  return 1;
}


/* Return 0 and populate @filename when Lua _VERSION. matches @ver */
static int find_anylua(char *filename, char* ver) {
  /* We memoize for the first Lua binary that returns a Version */
  static char ufile[PATH_MAX] = "\0";
  static char uver[100] = "\0";
  FILE *p;
  if (strcmp(ufile, "\0") == 0) {
    if (access(filename, X_OK) == 0 && check_lua_ver(filename, uver) == 0) {
      strcpy(ufile, filename);
    } else {
      filename=NULL;
      return 1;
    }
  }
  if (strcmp(ver, uver) == 0) {
    strcpy(filename, ufile);
    return 0;
  } else {
    filename=NULL;
    return 1;
  }
}

static int whereis_lua(char *res) {
  required_lua();
  for (int v=0; v < lua_ver.len; v++) {
    for (int p=0; p < path_list.len; p++) {
      /* Check if is lua5.1 .. lua5.4 */
      sprintf(res, "%s/%s%s", path_list.item[p], "lua", lua_ver.item[v].name);
      if (0 == access(res, X_OK)) return 0;

      /* Check if is lua51 .. lua54 */
      sprintf(res,"%s/%s%s", path_list.item[p], "lua", lua_ver.item[v].alt);
      if (0 == access(res, X_OK)) return 0;

      /* Check if is lua and matches Lua _VERSION */
      sprintf(res, "%s/%s", path_list.item[p], "lua");
      if(0 == find_anylua(res, lua_ver.item[v].name)) return 0;
    }
  }
  luft_error("No Lua version for the '%s' tree was found in your PATH", luft_root);
  return 1;
}


static void call_lua(char *bin, int argc, char **argv) {
  char *envargf = "LUFT_ARG[%d]";
  char *argl[argc+4];
  char envargn[sysconf(_SC_ARG_MAX)];

  /* arguments passed to Lua */
  argl[0] = bin;
  argl[1] = "-l";
  argl[2] = "luft";
  argl[3] = "--";
  argl[4] = luft_script;

  for( int i=1; i<argc; i++) argl[i+4]=argv[i];
  argl[argc+4]=NULL;

  /* arguments passed via env array */
  sprintf(envargn, envargf, 0);
  setenv(envargn, luft_script, 1);
  for( int i=1; i<argc; i++) {
    sprintf(envargn, envargf, i);
    setenv(envargn, argv[i], 1);
  }
  sprintf(envargn, envargf, -1);
  setenv(envargn, bin, 1);

  execvp(argl[0], argl);
  luft_error("'exec %s': %s", argl[0], strerror(errno));
}


/* resolve the real location for the script */
static void resolve(char *path) {
  struct stat st;
  char ps[PATH_MAX];
  char *dir;

  if (realpath(path, luft_script) == NULL)
    luft_error("Error accessing '%s' : %s", path, strerror(errno));

  stat(luft_script, &st);

  if (S_ISDIR(st.st_mode)) {
    strcat(luft_script,"/bin/main.lua");
    if ( stat(luft_script, &st) != 0 ) luft_error("Error accessing script '%s' : %s", luft_script, strerror(errno));
  }

  if (! (S_ISREG(st.st_mode))) luft_error("Error: '%s' must be a regular file", luft_script);

  strcpy(ps, luft_script);
  dir = dirname(ps);

  for (int i = strlen(dir); i>0; i--) {
    if (dir[i] == '/') {
      if (strcmp(&dir[i], "/bin") == 0) {
        dir[i]='\0';
        strcpy(luft_root, dir);
        break;
      }
    }
  }
  if ( luft_root[0] == '\0' ) {
    luft_error("The script '%s' doesn't belongs to a Luft tree", luft_script);
  }

  sprintf(luft_file,"%s/%s", luft_root, "luft");
  if (stat(luft_file,&st) != 0) luft_error("Error detecting Luft file at '%s' : %s", luft_file, strerror(errno));
  if( S_ISREG(st.st_mode) ) return;
  luft_error("The luft file wasn't found at '%s'", luft_root);
}


/* Process the shebanged file or directory */
static int luft_shell(int argc, char **argv) {
  char   bin[PATH_MAX];         /* the Lua executable */
  resolve(argv[0]);

  if (whereis_lua(bin) == 0) {
    printf("BIN %s\n", bin);
    call_lua(bin, argc, argv);
  }

  fprintf(stderr, "Not found a Lua version");
  return 1;
}


/* For future commands... check if string has dir separators */
static int ispath(char *str) {
  for (int i=0; str[i] != '\0'; i++)
    if (str[i] == '/')
      return 1;
  return 0;
}


int main(int argc, char **argv) {
  if (argc < 1) exit(1);
  if (ispath(argv[1]) ) {
      get_path_list();
      luft_shell(argc-1, &argv[1]);
  }
  return 0;
}

