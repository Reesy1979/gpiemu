


#ifndef __RC_H__
#define __RC_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum rctype
{
	rcv_end,
	rcv_int,
	rcv_float,
	rcv_string,
	rcv_vector,
	rcv_bool
} rcvtype_t;


typedef struct rcvar_s
{
	char *name;
	char *help;
	int type;
	int len;
	void *mem;
} rcvar_t;

#define RCV_END { 0, 0, rcv_end, 0, 0 }
#define RCV_INT(n,v,h) { (n), (h), rcv_int, 1, (v) }
#define RCV_FLOAT(n,v,h) { (n), (h), rcv_float, 1, (v) }
#define RCV_STRING(n,v,h) { (n), (h), rcv_string, 0, (v) }
#define RCV_VECTOR(n,v,l,h) { (n), (h), rcv_vector, (l), (v) }
#define RCV_BOOL(n,v,h) { (n), (h), rcv_bool, 1, (v) }

typedef struct rccmd_s
{
	char *name;
	int (*func)(int, char **);
} rccmd_t;

#define RCC(n,f) { (n), (f) }
#define RCC_END { 0, 0 }

void rc_export(rcvar_t *v);
void rc_exportvars(rcvar_t *vars);

int rc_findvar(char *name);

int rc_setvar_n(int i, int c, char **v);
int rc_setvar(char *name, int c, char **v);

int rc_getint_n(int i);
float rc_getfloat_n(int i);
int *rc_getvec_n(int i);
char *rc_getstr_n(int i);

int rc_getint(char *name);
float rc_getfloat(char *name);
int *rc_getvec(char *name);
char *rc_getstr(char *name);

int rc_command(char *line);
int rc_sourcefile(char *filename);
const char *rc_type_to_string(rcvtype_t type);

#ifdef __cplusplus
} // End of extern "C"
#endif

#endif




