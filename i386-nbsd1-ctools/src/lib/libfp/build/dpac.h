/* FPAC/DPAC system call (function) prototypes */


float	FPADD(float,float);		/* addition */
float	FPMUL(float,float);		/* multiplication */
float	FPDIV(float,float);		/* division */
float	FPRDIV(float,float);		/* division */
int	FPCMP(float,float);		/* comparison */
float	AINT(float);			/* convert float to float (int) */
int	INT(float);			/* convert float to int */
int	FIX(float);			/* convert float to int */
float	FLOAT(int);			/* convert int to float */
float	FPEXP(float);			/* e to float power */
float	FPLOG(float);			/* log 10 of float */
float	FPLN(float);			/* log e of float */
float	FPXTOI(float,int);		/* float to int power */
float	FPSQRT(float);			/* square root of float */
float	FPCOS(float);			/* cosine of float */
float	FPSIN(float);			/* sine of float */
float	FPTAN(float);			/* tangent of float */
float	FPATN(float);			/* arc tangent of float */
float	ASCBIN(char*);			/* convert ascii to float */
int	BINASC(float,char*);		/* convert float to ascii */

float	DPTOSP(double);			/* convert double to float */

float fpadd(float,float);
float fpsub(float,float);
float fpmul(float,float);
float fpdiv(float,float);
int fpcmp(float,float);
float fpflt(int);
int fpint(float);
int fpfix(float);
float	faint(float); 			/* floating floor */
float fascbin(char *, int *);
int	fbinasc(float, char *, int, int);
float fpexp(float);
float fpln(float);
float fplog(float);
float fpsin(float);
float fpcos(float);
float fpsqrt(float);
float fptan(float);
float fpxtoi(float,int);
float fpatn(float); 


double dpadd(double,double);
double dpsub(double,double);
double dpmul(double,double);
double dpdiv(double,double);
int dpcmp(double,double);
double dpflt(int);
int dpint(double);
double	daint(double);
int	dpfix(double);
double dascbin(char *, int *);
int	dbinasc(double, char *, int, int);
double sptodp(float);
float	dptosp(double);			/* convert double to float */

double dpexp(double);
double dpln(double);
double dplog(double);
double dpsin(double);
double dpcos(double);
double dpsqrt(double);
double dptan(double);
double dpxtoi(double,int);
double dpatn(double); 




double	DOUBLE(float);			/* convert float to double */

double	DPADD(double,double);		/* addition */		
double	DPMUL(double,double);		/* multiplication */		
double	DPDIV(double,double);		/* division */		
double	DPRDIV(double,double);		/* division */		
int		DPCMP(double,double);		/* comparision */		
double	DAINT(double);			/* convert double to double (int) */
int		DINT(double);			/* convert double to int */
int		DFIX(double);			/* convert double to int */
double	DFLOAT(long);			/* convert long to double */
double	DPEXP(double);			/* e to double power */		
double	DPLOG(double);			/* log 10 of double */
double 	DPLN(double);			/* log e of double */
double	DPXTOI(double,int);		/* double to int power */
double	DPSQRT(double);			/* square root of double */
double	DPCOS(double);			/* cosine of double */	
double	DPSIN(double);			/* sine of double */
double	DPTAN(double);			/* tangent of double */
double	DPATN(double);			/* arc tangent of double */
double	DASCBN(char*);			/* convert ascii to double */	
int	DBNASC(double,char*);	 	/* convert double to ascii */
