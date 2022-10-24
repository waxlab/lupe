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



static char lupe_script[PATH_MAX] = "\0"; /* Lua file with #! pointing to lupe*/
static char lupe_rc[PATH_MAX]     = "\0"; /* the directory meta specifications */
static char lupe_root[PATH_MAX]   = "\0"; /* the tree root directory */

enum lupe_mode { lupe_mode_cli, lupe_mode_dir };

/* Lua binary required */
struct lupe_lua {
  char bin[PATH_MAX]; /* Lua binary path */
  char version[10];
};

/* List containing the required Lua versions */
struct lupe_rc_lua {
  int verc;
  struct {
    char version[10]; /* Ex: 5.4 */
    char altvers[10]; /* Ex: 54 (BSD and other Unixes) */
  } verv[6];
};

static struct {
  char item[100][PATH_MAX];
  int  len;
} path_list;

/* matches the line version */
static char const *lv_exp =
  "lua = %*[\"{ ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]";


static void lupe_error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args); fprintf(stderr,"\n");
  va_end(args);
  exit(1);
}


/* Convert the environment PATH in a list of directories */
static int get_path_list(void) {
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
    item = strtok(path, ":");
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
static void validate_lua_ver(struct lupe_rc_lua rc_lua) {
  char altvers[100] = "\0"; /* if ver="5.4" this will be "54" */
  int  ai       = 0;
  char *version;

  for(int v=0; v < rc_lua.verc; v++) {
    version=rc_lua.verv[v].version;
    for(int c=0; c<strlen(version); c++) {
      if (version[c] >= '0' && version[c] <= '9') {
        altvers[ai++] = version[c];
        continue;
      }
      if (version[c] != '.') lupe_error("Invalid version %s", version);
    }
    altvers[ai]='\0';
    strcpy(rc_lua.verv[v].altvers, altvers);
  }
}



/* Read file line by line until find the Lua required version */
static struct lupe_rc_lua get_lupe_rc_lua(void) {
  FILE  *fp = fopen(lupe_rc, "r");
  char  *line  = NULL;
  size_t linelen = 0;
  int    linenum = 0;
  int    vv = 0;
  static struct lupe_rc_lua rc_lua;
  rc_lua.verv[0].version[0] = '\0';
  rc_lua.verc = 0;

  if (fp) {
    while (getline(&line, &linelen, fp) != -1 && linenum++ < 3) {
      vv = sscanf(line, lv_exp,
                  rc_lua.verv[0].version, rc_lua.verv[1].version,
                  rc_lua.verv[2].version, rc_lua.verv[3].version,
                  rc_lua.verv[4].version, rc_lua.verv[5].version);
      if (vv) {
        rc_lua.verc = vv;
        validate_lua_ver(rc_lua);
        break;
      }
    }
    free(line);
    fclose(fp);

    if (rc_lua.verv[0].version[0] == '\0') {
      lupe_error("no valid 'lua' entry found at the beginning of '%s'", lupe_rc);
    }
  }
  return rc_lua;
}



/* Returns 0 when can successfully run the Lua binary and get _VERSION */
static int check_lua_ver(char *bin, char *ver) {
  char cmd[PATH_MAX];
  FILE *p;
  sprintf(cmd, "%s -e 'print(_VERSION)'", bin);
  p = popen(cmd, "r");
  if (p != NULL) {
    if (fscanf(p, "%*s %[0-9.]", ver) > 0) {
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



static struct lupe_lua whereis_lua(void) {
  struct lupe_rc_lua rc_lua = get_lupe_rc_lua();
  struct lupe_lua    lua;
  strcpy(lua.bin,    "\0");
  strcpy(lua.version,"\0");

  int v=0; /* keep at this scope! */
  for ( ; v < rc_lua.verc; v++) {
    for (int p=0; p < path_list.len; p++) {
      /* Check if is lua5.1 .. lua5.4 */
      sprintf(lua.bin, "%s/%s%s", path_list.item[p], "lua", rc_lua.verv[v].version);
      if (0 == access(lua.bin, X_OK)) goto found;

      /* Check if is lua51 .. lua54 */
      sprintf(lua.bin,"%s/%s%s", path_list.item[p], "lua", rc_lua.verv[v].altvers);
      if (0 == access(lua.bin, X_OK)) goto found;

      /* Check if is lua and matches Lua _VERSION */
      sprintf(lua.bin, "%s/%s", path_list.item[p], "lua");
      if(0 == find_anylua(lua.bin, rc_lua.verv[v].version)) goto found;
    }
  }
  lupe_error("No Lua version for the '%s' tree was found in your PATH", lupe_root);

  found:
    strcpy(lua.version, rc_lua.verv[v].version);
    return lua;
}

/* If string terminates with \n remove it and returns */
static char *trimnl(char *s) {
  int l = strlen(s);
  if (s[l-1] == '\n') s[l-1]='\0';
  return s;
}


/* ENVIRONMENT VARIABLES PASSED TO LUA
** Lua 5.1 and 5.2 does not initializes the _G.arg table until the Lua file
** is processed. For the lupe module has access to the argument list, it is
** emulated trough the LUPE_ARG[*] set of variables.
*/
static void lupe_env(struct lupe_lua lua, int argc, char **argv) {
  char *lua_argf = "LUA_ARG[%d]";
  char *lua_argi = malloc(sizeof(char) * sysconf(_SC_ARG_MAX));

  setenv("LUPE_ROOT", lupe_root, 1);
  setenv("LUPE_RC", lupe_rc, 1);

  sprintf(lua_argi, lua_argf, 0);
  setenv(lua_argi, lupe_script, 1);
  for(int i=0; i < argc; i++) {
    sprintf(lua_argi, lua_argf, i);
    setenv(lua_argi, argv[i], 1);
  }
  /* interpreter is the -1 */
  sprintf(lua_argi, lua_argf, -1);
  setenv(lua_argi, lua.bin, 1);

  /* unset the argc+1... the principle of the non-sparse list ended in nil */
  sprintf(lua_argi, lua_argf, argc);
  unsetenv(lua_argi);
  free(lua_argi);
}



static void call_lupe_lua(struct lupe_lua lua, enum lupe_mode mode, int argc, char **argv) {
  char *argl[1024];
  /* arguments passed to Lua */
  argl[0] = lua.bin;
  argl[1] = "-l";
  argl[3] = "--";
  if (mode == lupe_mode_cli) {
    argl[2] = "lupe.cli";
    argl[4] = "/dev/null";
  } else {
    argl[2] = "lupe.dir";
    argl[4] = lupe_script;
  }

  for( int i=1; i<argc; i++) argl[i+4]=argv[i];
  lupe_env(lua, argc, argv);
  execvp(argl[0], argl);
  lupe_error("'exec %s': %s", argl[0], strerror(errno));
}

static void set_lupe_script(char *path) {
  struct stat st;
  if (realpath(path,lupe_script) == NULL) {
    lupe_error("Error setting lupe_script from '%s' : %s", path, strerror(errno));
  }
}

/* resolve the real location for the script */
static void resolve(char *path) {
  struct stat st;
  char fullpath[PATH_MAX];
  if ( realpath(path, fullpath) == NULL ) {
    lupe_error("%s: %s", path, strerror(errno));
  }

  stat (fullpath, &st);
  if (S_ISDIR(st.st_mode)) {
    strcpy(lupe_root, path);
    sprintf(lupe_script, "%s/bin/main.lua", lupe_root);
  } else if (S_ISREG(st.st_mode)) {
    strcpy(lupe_script, fullpath);
    char *dir = dirname(fullpath);
    for (int i = strlen(dir); i>0; i--) {
      if (dir[i] == '/') {
        if (strcmp(&dir[i], "/bin") == 0) {
          dir[i]='\0';
          strcpy(lupe_root, dir);
          break;
        }
      }
    }
    if ( lupe_root[0] == '\0' ) {
      lupe_error("The script '%s' doesn't belongs to a Lupe tree", lupe_script);
    }
  } else {
    lupe_error("Error: '%s' must be a regular file", lupe_script);
  }
  sprintf(lupe_rc, "%s/luperc.lua", lupe_root);
  if (stat(lupe_rc, &st) != 0) {
    lupe_error("Error detecting '%s' : %s", lupe_rc, strerror(errno));
  }
  if (! S_ISREG(st.st_mode) ) {
    lupe_error("The lupe file wasn't found at '%s'", lupe_root);
  }
}



/* For future commands... check if string has dir separators */
static int ispath(char *str) {
  for (int i=0; str[i] != '\0'; i++)
    if (str[i] == '/')
      return 1;
  return 0;
}


int main(int argc, char **argv) {

  char target_path[PATH_MAX];
  struct lupe_lua lua;
  enum lupe_mode mode;
  get_path_list();

  if (argc > 1 && ispath(argv[1])) {
    mode = lupe_mode_dir;
    if (realpath(argv[1], target_path) == NULL) {
      lupe_error("Error resolving path to '%s' : %s", target_path, strerror(errno));
    }
    argc--; argv = &argv[1];
    resolve(argv[0]);
    lua = whereis_lua();
  } else {
    getcwd(target_path,PATH_MAX);
    mode = lupe_mode_cli;
    /* For other cli commands we need the location of luperc.lua file. */
    if (argc > 1 && strcmp(argv[1],"help") != 0 && strcmp(argv[1], "init")) {
      resolve(target_path);
    }
    strcpy(lua.bin,     "lua");
    strcpy(lua.version, "\0");
  }
  call_lupe_lua( lua, mode, argc, argv );
  return 0;
}

