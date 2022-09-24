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


static struct {
  char bin[PATH_MAX]; /* Lua binary path */
  char version[10];

  /* List containing the required Lua versions */
  int verc;
  struct {
    char version[10]; /* Ex: 5.4 */
    char altvers[10]; /* Ex: 54 (BSD and other Unixes) */
  } verv[6];
} Lua;


static struct {
  char item[100][PATH_MAX];
  int  len;
} path_list;

static char lupe_script[PATH_MAX] = "\0"; /* Lua file with #! pointing to lupe*/
static char lupe_rc[PATH_MAX]   = "\0"; /* the directory meta specifications */
static char lupe_root[PATH_MAX]   = "\0"; /* the tree root directory */
static char ERROR[1024];

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
static void validate_lua_ver(void) {
  char altvers[100] = "\0"; /* if ver="5.4" this will be "54" */
  int  ai       = 0;
  char *version;

  for(int v=0; v < Lua.verc; v++) {
    version=Lua.verv[v].version;
    for(int c=0; c<strlen(version); c++) {
      if (version[c] >= '0' && version[c] <= '9') {
        altvers[ai++] = version[c];
        continue;
      }
      if (version[c] != '.') lupe_error("Invalid version %s", version);
    }
    altvers[ai]='\0';
    strcpy(Lua.verv[v].altvers, altvers);
  }
}



/* Read file line by line until find the Lua required version */
static void required_lua(void) {
  FILE  *fp = fopen(lupe_rc, "r");
  char  *line  = NULL;
  size_t linelen = 0;
  int    linenum = 0;
  int    vv = 0;
  Lua.verv[0].version[0] = '\0';
  Lua.verc = 0;

  if (fp) {
    while (getline(&line, &linelen, fp) != -1 && linenum++ < 3) {
      vv = sscanf(line, lv_exp,
                  Lua.verv[0].version, Lua.verv[1].version,
                  Lua.verv[2].version, Lua.verv[3].version,
                  Lua.verv[4].version, Lua.verv[5].version);
      if (vv) {
        Lua.verc = vv;
        validate_lua_ver();
        break;
      }
    }
    free(line);
    fclose(fp);

    if (Lua.verv[0].version[0] == '\0') {
      lupe_error("no valid 'lua' entry found at the beginning of '%s'", lupe_rc);
    }
  }
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

static int whereis_lua(void) {
  required_lua();
  int v=0; /* keep at this scope! */
  for ( ; v < Lua.verc; v++) {
    for (int p=0; p < path_list.len; p++) {
      /* Check if is lua5.1 .. lua5.4 */
      sprintf(Lua.bin, "%s/%s%s", path_list.item[p], "lua", Lua.verv[v].version);
      if (0 == access(Lua.bin, X_OK)) goto found;

      /* Check if is lua51 .. lua54 */
      sprintf(Lua.bin,"%s/%s%s", path_list.item[p], "lua", Lua.verv[v].altvers);
      if (0 == access(Lua.bin, X_OK)) goto found;

      /* Check if is lua and matches Lua _VERSION */
      sprintf(Lua.bin, "%s/%s", path_list.item[p], "lua");
      if(0 == find_anylua(Lua.bin, Lua.verv[v].version)) goto found;
    }
  }
  lupe_error("No Lua version for the '%s' tree was found in your PATH", lupe_root);
  return 1;
  found:
    strcpy(Lua.version, Lua.verv[v].version);
    return 0;
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
static void lupe_env(int argc, char **argv) {
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
  setenv(lua_argi, Lua.bin, 1);

  /* unset the argc+1... the principle of the non-sparse list ended in nil */
  sprintf(lua_argi, lua_argf, argc);
  unsetenv(lua_argi);
  free(lua_argi);
}



static void call_lua(int argc, char **argv) {
  char *argl[1024];
  /* arguments passed to Lua */
  argl[0] = Lua.bin;
  argl[1] = "-l";
  argl[3] = "--";
  if (lupe_script[0] == '\0') {
    argl[2] = "lupe.cli";
    argl[4] = "/dev/null";
  } else {
    argl[2] = "lupe.dir";
    argl[4] = lupe_script;
  }

  for( int i=1; i<argc; i++) argl[i+4]=argv[i];
  lupe_env(argc, argv);
  execvp(argl[0], argl);
  lupe_error("'exec %s': %s", argl[0], strerror(errno));
}


/* resolve the real location for the script */
static void resolve(char *path) {
  struct stat st;
  char ps[PATH_MAX];
  char *dir;

  if (realpath(path, lupe_script) == NULL)
    lupe_error("Error accessing '%s' : %s", path, strerror(errno));

  stat(lupe_script, &st);

  if (S_ISDIR(st.st_mode)) {
    strcat(lupe_script,"/bin/main.lua");
    if ( stat(lupe_script, &st) != 0 ) lupe_error("Error accessing script '%s' : %s", lupe_script, strerror(errno));
  }

  if (! (S_ISREG(st.st_mode))) lupe_error("Error: '%s' must be a regular file", lupe_script);

  strcpy(ps, lupe_script);
  dir = dirname(ps);

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

  sprintf(lupe_rc, "%s/%s", lupe_root, "luperc.lua");
  if ( stat(lupe_rc, &st) != 0 ) {
    lupe_error("Error detecting '%s' : %s", lupe_rc, strerror(errno));
  }
  if( S_ISREG(st.st_mode) ) return;
  lupe_error("The lupe file wasn't found at '%s'", lupe_root);
}



/* Process the shebanged file or directory */
static int lupe_shell(int argc, char **argv) {
  resolve(argv[0]);

  if (whereis_lua() == 0) {
    call_lua(argc, argv);
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
  /*
  ** Lua requires a filename to be processed after or it falls
  ** into the repl. So lupe_script should be a Lua file or /dev/null
  */

  strcpy(Lua.bin,    "\0");
  strcpy(Lua.version,"\0");
  Lua.verc = 0;

  if ( argc > 1 && ispath(argv[1]) ) {
    get_path_list();
    lupe_shell(argc-1, &argv[1]);
    return 0;
  }

  char *argvdef[2] = { argv[0], "help" };
  if (argc < 2) {
    argc = 2;
    argv = argvdef;
  }
  getcwd(lupe_root, PATH_MAX);
  sprintf(lupe_rc, "%s/%s", lupe_root, "luperc.lua");
  strcpy(Lua.bin, "lua");
  call_lua(argc, argv);
  return 0;
}

