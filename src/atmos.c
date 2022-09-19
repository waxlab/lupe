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
  char bin[PATH_MAX]; /* Lua binary path */
  char *version;      /* Lua binary version found */

  /* List containing the required Lua versions */
  int verc; struct {
    char name[4]; /* Ex: 5.4 */
    char alt[10]; /* Ex: 54 (BSD and other Unixes) */
  } verv[6];
} Lua;


struct {
  char item[100][PATH_MAX];
  int  len;
} path_list;

static char atmos_script[PATH_MAX] = "\0"; /* Lua file with #! pointing to atmos*/
static char atmos_config[PATH_MAX] = "\0"; /* the tre conf */
static char atmos_root[PATH_MAX]   = "\0"; /* the tree root directory */
static char ERROR[1024];

/* matches the line version */
static char const *lv_exp =
  "lua = %*[\"{ ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]%*[,\" ]%[0-9.]";


static void atmos_error(char *fmt, ...) {
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
static void validate_lua_ver() {
  char alt[100] = "\0"; /* if ver="5.4" this will be "54" */
  int  ai       = 0;
  char *name;

  for(int v=0; v < Lua.verc; v++) {
    name=Lua.verv[v].name;
    for(int c=0; c<strlen(name); c++) {
      if (name[c] >= '0' && name[c] <= '9') {
        alt[ai++] = name[c];
        continue;
      }
      if (name[c] != '.') atmos_error("Invalid version %s", name);
    }
    alt[ai]='\0';
    strcpy(Lua.verv[v].alt, alt);
  }
}



/* Read file line by line until find the Lua required version */
static void required_lua() {
  FILE  *fp = fopen(atmos_config, "r");
  char  *line  = NULL;
  size_t linelen = 0;
  int    linenum = 0;
  int    vv = 0;
  Lua.verv[0].name[0] = '\0';
  Lua.verc = 0;

  if (fp) {
    while (getline(&line, &linelen, fp) != -1 && linenum++ < 3) {
      vv = sscanf(line, lv_exp,
                  Lua.verv[0].name, Lua.verv[1].name,
                  Lua.verv[2].name, Lua.verv[3].name,
                  Lua.verv[4].name, Lua.verv[5].name);
      if (vv) {
        Lua.verc = vv;
        validate_lua_ver();
        break;
      }
    }
    free(line);
    fclose(fp);

    if (Lua.verv[0].name[0] == '\0') {
      atmos_error("no valid 'lua' entry found at the beginning of '%s'", atmos_config);
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

static int whereis_lua() {
  required_lua();
  int v=0; /* keep at this scope! */
  for ( ; v < Lua.verc; v++) {
    for (int p=0; p < path_list.len; p++) {
      /* Check if is lua5.1 .. lua5.4 */
      sprintf(Lua.bin, "%s/%s%s", path_list.item[p], "lua", Lua.verv[v].name);
      if (0 == access(Lua.bin, X_OK)) goto found;

      /* Check if is lua51 .. lua54 */
      sprintf(Lua.bin,"%s/%s%s", path_list.item[p], "lua", Lua.verv[v].alt);
      if (0 == access(Lua.bin, X_OK)) goto found;

      /* Check if is lua and matches Lua _VERSION */
      sprintf(Lua.bin, "%s/%s", path_list.item[p], "lua");
      if(0 == find_anylua(Lua.bin, Lua.verv[v].name)) goto found;
    }
  }
  notfound:
    atmos_error("No Lua version for the '%s' tree was found in your PATH", atmos_root);
    return 1;
  found:
    Lua.version = Lua.verv[v].name;
    return 0;
}


/* ENVIRONMENT VARIABLES PASSED TO LUA
** Lua 5.1 and 5.2 does not initializes the _G.arg table until the Lua file
** is processed. For the atmos module has access to the argument list, it is
** emulated trough the ATMOS_ARG[*] set of variables.
*/
static void atmos_env(int argc, char **argv) {
  char *lua_argf = "LUA_ARG[%d]";
  char *lua_argi = malloc(sizeof(char) * sysconf(_SC_ARG_MAX));

  setenv("ATMOS_ROOT", atmos_root, 1);
  setenv("ATMOS_CONFIG", atmos_config, 1);

  sprintf(lua_argi, lua_argf, 0);
  setenv(lua_argi, atmos_script, 1);
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
  char *argl[argc+4];
  /* arguments passed to Lua */
  argl[0] = Lua.bin;
  argl[1] = "-l";
  argl[3] = "--";
  if (atmos_script[0] == '\0') {
    argl[2] = "atmos.cli";
    argl[4] = "/dev/null";
  } else {
    argl[2] = "atmos.sbi.main";
    argl[4] = atmos_script;
  }

  for( int i=1; i<argc; i++) argl[i+4]=argv[i];
  atmos_env(argc, argv);
  execvp(argl[0], argl);
  atmos_error("'exec %s': %s", argl[0], strerror(errno));
}


/* resolve the real location for the script */
static void resolve(char *path) {
  struct stat st;
  char ps[PATH_MAX];
  char *dir;

  if (realpath(path, atmos_script) == NULL)
    atmos_error("Error accessing '%s' : %s", path, strerror(errno));

  stat(atmos_script, &st);

  if (S_ISDIR(st.st_mode)) {
    strcat(atmos_script,"/bin/main.lua");
    if ( stat(atmos_script, &st) != 0 ) atmos_error("Error accessing script '%s' : %s", atmos_script, strerror(errno));
  }

  if (! (S_ISREG(st.st_mode))) atmos_error("Error: '%s' must be a regular file", atmos_script);

  strcpy(ps, atmos_script);
  dir = dirname(ps);

  for (int i = strlen(dir); i>0; i--) {
    if (dir[i] == '/') {
      if (strcmp(&dir[i], "/bin") == 0) {
        dir[i]='\0';
        strcpy(atmos_root, dir);
        break;
      }
    }
  }
  if ( atmos_root[0] == '\0' ) {
    atmos_error("The script '%s' doesn't belongs to a Atmos tree", atmos_script);
  }

  sprintf(atmos_config, "%s/%s", atmos_root, "atmospec.lua");
  if ( stat(atmos_config, &st) != 0 ) {
    atmos_error("Error detecting '%s' : %s", atmos_config, strerror(errno));
  }
  if( S_ISREG(st.st_mode) ) return;
  atmos_error("The atmos file wasn't found at '%s'", atmos_root);
}



/* Process the shebanged file or directory */
static int atmos_shell(int argc, char **argv) {
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
  if (argc < 2) return 1;

  if (ispath(argv[1]) ) {
    get_path_list();
    atmos_shell(argc-1, &argv[1]);
  } else {
    /* Lua requires a filename to be processed after or it falls
       into the repl. So atmos_script should be a Lua file or /dev/null */
    getcwd(atmos_root, PATH_MAX);
    sprintf(atmos_config, "%s/%s", atmos_root, "atmospec.lua");
    strcpy(Lua.bin, "lua");
    call_lua(argc, argv);
  }
  return 0;
}

