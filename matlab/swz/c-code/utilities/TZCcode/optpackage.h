/************ 3 steps to find minimization solution. *****************
 * See details at the bottom of this file.
 *   or lwz_est.c in D:\ZhaData\WorkDisk\LiuWZ\Project2_empirical\EstimationOct07
 *   or ExamplesForC.prn in D:\ZhaData\CommonFiles\C_Examples_DebugTips
 *
 *
 * 1. minpack_csminwel_ps = CreateTSminpack();
 * 2. InitializeForMinproblem(minpack_csminwel_ps, ..., indxRanIniForMin);
 *        //This is a local, project-specific function that initializes minpack_csminwel_ps->x_dv (note, NOT xtemp_dv)
 *        //  according to indxStartValuesForMin.
 * 3. minfinder(minpack_csminwel_ps);
/*********************************************************************/


#ifndef __OPTPACKAGE_H__
#define __OPTPACKAGE_H__

#include "tzmatlab.h"
#include "csminwel.h"
#include "congradmin.h"
#include "fn_filesetup.h"  //fn_SetFilePosition(), etc.
#include "mathlib.h"      //CopyVector0(), etc.
#include "switch_opt.h"    //DW's optimization routines for Markov-switching models.
#include "cstz.h" //Used for gradcd_gen() only in the IMSL linear constrainted problem.

//-------------- Attributes for selecting optimization packages. --------------
#define MIN_DEFAULT   0        //0 or NULL: default or no minimization package.
#define MIN_CSMINWEL  0x0001   //1: csminwel unconstrained minimization package.
#define MIN_IMSL      0x0002   //2: IMSL unconstrained minimization package.
#define MIN_IMSL_LNCONS   0x0004   //4: IMSL linearly constrained minimization package.
#define MIN_IMSL_NLNCONS   0x0008   //8: IMSL nonlinearly constrained minimization package.
#define MIN_CONGRADI    0x0010   //16: unconstrained conjugate gradient minimization method 1.  Polak-Ribiere conjugate gradient method without using derivative information in performing the line minimization.
#define MIN_CONGRADII   0x0020  //2*16=32: unconstrained conjugate gradient minimization method 2.  NOT available yet! Pletcher-Reeves conjugate gradient method using derivative information in performing the line minimization.
//#define MIN_CONGRADII   0x0040  //4*16=2^6: unconstrained conjugate gradient minimization method 2.
//#define MIN_CONGRADII   0x0080  //8*16=2^7: unconstrained conjugate gradient minimization method 2.
//#define MIN_CONGRADII   0x0100  //16^2=2^8: unconstrained conjugate gradient minimization method 2.


//-------------- Minimization package: unconstrained BFGS csminwel. --------------
//--- The following three macros will be void if the input data file specifies the values of these macros.
//--- The following three are used for the constant-parameter model only.
#define CRIT_CSMINWEL   1.0e-09     //1.5e-08 (for monthly TVBVAR)    //Overall convergence criterion for the function value.
#define ITMAX_CSMINWEL  100000       //Maximum number of iterations.
#define INI_H_CSMINWEL   1.0e-005  //Initial value for the diagonal of inverse Hessian in the quasi-Newton search.
                                  //1.0e-05 (sometimes used for SargentWZ USinflation project I)
                                  //5.0e-04 (for monthly TVBAR)
//--- The following macros are used in csminwel.c.  Have not got time to make them void by input values.
#define INDXNUMGRAD_CSMINWEL 2         //Index for choosing the numerical gradient.  1, forward difference; 2, central difference.
                             //central difference method is twice as slower as forward difference.


//-------------- Minimization package: linearly-nconstrained IMSL. --------------
#define CRIT_IMSLCONLIN   1.0e-09      //Overall convergence criterion on the first-order conditions.
#define ITMAX_IMSLCONLIN  100000       //Maximum number of iterations.

//-------------- Minimization package: conjugate gradient method I. --------------
#define CRIT_CONGRAD1   1.0e-09      //Overall convergence criterion on the first-order conditions.
#define ITMAX_CONGRAD1  100000       //Maximum number of iterations.


//struct TSminpack_tag;

// extern struct TSminpack_tag *MINPACK_PS;


//typedef void TFminfinder(struct TSminpack_tag *, const int ipackage);  //If ipackage = MIN_CWMINWEL, uses csminwel; etc.
//int n, double *x_ptr, double g_ptr);  //, void *mingrad_etc_ptr);
//typedef void TFmingrad_imsl(struct TSminpack_tag *);  //NOT used yet.
//typedef void TFmingrad(void);   //int n, double *x_ptr, double g_ptr);  //, void *mingrad_etc_ptr);

//======================================================
// Old way of using cwminwel.  No longer used in my new code. 11/01/05.
//======================================================
//------- For unconstrained BFGS csminwel only. -------
typedef struct TSetc_csminwel_tag {
   //=== Optional input arguments (originally set up by Iskander), often or no longer NOT used, so we set to NULL at this point.
   double **args; //k-by-q.
   int *dims;   //k-by-1;
   int _k;

   //=== Mandatory input arguments.
   TSdmatrix *Hx_dm;  //n-by-n inverse Hessian.  Output as well, when csminwel is done.
   double crit;   //Overall convergence criterion for the function value.
   int itmax;  //Maximum number of iterations.

   //=== Some reported input arguments.
   double ini_h_csminwel;
   int indxnumgrad_csminwel;
   double gradstps_csminwel;  //Step size for the numerical gradient if no analytical gradient is available.


   //=== Output arguments.
   int badg;    //If (badg==0), analytical gradient is used; otherwise, numerical gradient will be produced.
   int niter;   //Number of iterations taken by csminwel.
   int fcount;   //Number of function evaluations used by csminwel.
   int retcode;  //Return code for the terminating condition.
                // 0, normal step (converged). 1, zero gradient (converged).
                // 4,2, back and forth adjustment of stepsize didn't finish.
                // 3, smallest stepsize still improves too slow. 5, largest step still improves too fast.
                // 6, no improvement found.
} TSetc_csminwel;


//=============================================================
// New ways of making optimization packages.
//=============================================================
typedef struct TSminpack_tag {
   //=== Input arguments.
   int package;  //Minimization package or routine.
   TSdvector *x_dv;   //n-by-1 of estimated parameters.
   TSdvector *g_dv;   //n-by-1 of gradient. When no analytical gradient is provided, it returns the numerical one.
   //$$$$ The x_dv and g_dv are only used minfinder().  In the wrapper function like minobj_csminwelwrap(), we must
   //$$$$   use xtemp_dv and gtemp_dv to be repointed to the temporary array created in csminwel() itself.  See below.

   TSdvector *xtemp_dv;  //$$$$Used within the minimization problem.
   TSdvector *gtemp_dv;  //$$$$Used within the minimization problem.
   //$$$$WARNING: Note the vector xtemp_dv->v or gtemp_dv-v itself is not allocated memory, but only the POINTER.
   //$$$$           Within the minimization routine like csminwel(), the temporary array x enters as the argument in
   //$$$$           the objective function to compare with other values.  If we use minpack_ps->x_dv->v = x
   //$$$$           in a wrapper function like minobj_csminwelwrap() where x is a temporay array in csminwel(),
   //$$$$           this tempoary array (e.g., x[0] in csminwel()) within the csminwel minimization routine
   //$$$$           will be freed after the csminwel minimization is done.  Consequently, minpack_ps->x_dv-v, which
   //$$$$           which was re-pointed to this tempoary array, will freed as well.  Thus, no minimization results
   //$$$$           would be stored and trying to access to minpack_ps->x_dv would cause memory leak.
   //$$$$           We don't need, however, to create another temporary pointer within the objective function itself,
   //$$$$           but we must use minpack_ps->xtemp_dv for a *wrapper* function instead and at the end of
   //$$$$           minimization, minpack_ps->x_dv will have the value of minpack_ps->xtemp_dv, which is automatically
   //$$$$           taken care of by csminwel with the lines such as
   //$$$$                 memcpy(xh,x[3],n*sizeof(double));
   //$$$$           where xh and minpack_ps->x_dv->v point to the same memory space.

   TSdvector *x0_dv;  //n-by-1 of initial or starting values of the estimated parameters.


   //--- Created here.  Contains csminwel arguments iter, retcodeh, etc. or those that are essential to minimization package.
   void *etc_package_ps;

   //--- Created outside of this structure.  Including, say, csminwel input arguments such as convergence criteria
   //---   or block-wise csminwel input arguments.
   void *etc_project_ps;
   void *(*DestroyTSetc_project)(void *);
   //--- Optional.
   char *filename_printout;

   //--- Minimization function for objective function.
   //--- May *NOT* be needed for swithcing model because DW's switch_opt.c takes care of things.
   double (*minobj)(struct TSminpack_tag *);    ////From the input argument of CreateTSminpack().
          /*** This function is used only for the constant-parameter case, NOT for DW's Markov-swtiching case. ***/
   //--- Optional: Minimization function for analytical gradient. Often NOT available.
   void (*mingrad)(struct TSminpack_tag *);   //From the input argument of CreateTSminpack().

   //=== Output arguments.
   double fret;   //Returned value of the objective function.
   double fret0;  //Returned value of the objective function at the initial or starting values x0.

} TSminpack;

typedef double TFminobj(struct TSminpack_tag *);  //int n, double *x_ptr);  //, void *minobj_etc_ptr);
typedef void TFmingrad(struct TSminpack_tag *);
typedef void *TFmindestroy_etcproject(void *);
typedef void TFSetPrintFile(char *);

//======= Function prototypes. =======//
TSminpack *CreateTSminpack(TFminobj *minobj_func, void **etc_project_pps, TFmindestroy_etcproject *etcproject_func, TFmingrad *mingrad_func, char *filename_printout, const int n, const int package);
TSminpack *DestroyTSminpack(TSminpack *);


//=== Used for the constant-parameter model.
//--- 28/Oct/07: The function InitializeForMinproblem_const() has not been used even for the constant-parameter model.
//---   For examples, see lwz_est.c in D:\ZhaData\WorkDisk\LiuWZ\Project2_empirical\EstimationOct07
//---                  or ExamplesForC.prn under D:\ZhaData\CommonFiles\C_Examples_DebugTips.
//NOT used:  void InitializeForMinproblem_const(struct TSminpack_tag *minpack_ps, char *filename_sp, TSdvector *gphi_dv, int indxStartValuesForMin);
//---
void minfinder(TSminpack *minpack_ps);


//------------------------------------------------------------------------------//
//----------    New ways of making optimization packages. 03/10/06.      -------//
//------------------------------------------------------------------------------//
//================ For the csminwel minimization problem. ================//
//=== Step 1.
typedef struct TSargs_blockcsminwel_tag
{
   //Arguments for blockwise minimization.

   //=== Within the block: sequence of convergence criteria.
   double criterion_start; //Default: 1.0e-3;
   double criterion_end;  //Default: 1.0e-6;
   double criterion_increment; //Default: 0.1;
   int max_iterations_start;  //Default: 50;  Max # of iterations for csminwel.  The starting value is small because criterion_start
                              //  is coarse at the start.  As the convergence criterion is getting tighter, the max # of
                              //  iteration increases as it is multiplied by max_iterations_increment.
   double max_iterations_increment; //Default: 2.0; Used to multiply the max # of iterations in csminwel as the convergence
                                    //  criterion tightens.
   double ini_h_scale;  //Default: 5.0e-4;  1.0e-05 (sometimes used for SargentWZ USinflation project I)
                        //                  5.0e-04 (for monthly TVBAR)
   //=== Outside the blocks.
   int max_block_iterations;  //Default: 100;

   //------------------------------------------------------------
   //Step size for numerical gradient only when the value of x is less than 1.0 in absolute value.
   //If abs(x)>1.0, the step size is  GRADSTPS_CSMINWEL*x.
   //
   //For the time-varying-parameter model, GRADSTPS_CSMINWEL takes the values of gradstps_csminwel_dv:
   // 1st element: gradient step for the model parameters (tends to be large; the default value is 1.0e-02).
   // 2nd element: gradient step for the transition probability matrix (tends to be smaller; the default value is 1.0e-03)
   // 3rd element: gradient step for all the parameters (tends to be smaller; the default value is 1.0e-03 or 1.0e-04).
   //For the constant-parameter model:
   // GRADSTPS_CSMINWEL takes the value of gradstps_csminwel_const.  The default value is 1.0e-04 (for monthly TBVAR)
   //------------------------------------------------------------
   TSdvector *gradstps_csminwel_dv;  //3-by-1.  For the time-varying-parameter model only.
   double gradstps_csminwel_const;  //For the constant-parameter model.

   //--- pointer to the input data file that contains all the data on convergence, max iterations, etc.
   FILE *fptr_input1;
} TSargs_blockcsminwel;
struct TSargs_blockcsminwel_tag *CreateTSargs_blockcsminwel(FILE *fptr_input1);
             //If fptr_input1==NULL or no no values supplied when fptr_input1 != NULL, default values are taken.
struct TSargs_blockcsminwel_tag *DestroyTSargs_blockcsminwel(struct TSargs_blockcsminwel_tag *args_blockcsminwel_ps);
//+
typedef struct TStateModel_tag *TFDestroyTStateModel(struct TStateModel_tag *);
typedef struct TSetc_minproj_tag
{
   //For optimization of the posterior or likelihood function.
   struct TStateModel_tag *smodel_ps;
   struct TStateModel_tag *(*DestroyTStateModel)(struct TStateModel_tag *);
   //
   struct TSargs_blockcsminwel_tag *args_blockcsminwel_ps;
   struct TSargs_blockcsminwel_tag *(*DestroyTSargs_blockcsminwel)(struct TSargs_blockcsminwel_tag *);
} TSetc_minproj;
//
struct TSetc_minproj_tag *CreateTSetc_minproj(struct TStateModel_tag **smodel_pps, TFDestroyTStateModel *DestroyTStateModel_func,
                  struct TSargs_blockcsminwel_tag **args_blockcsminwel_pps, struct TSargs_blockcsminwel_tag *(*DestroyTSargs_blockcsminwel)(struct TSargs_blockcsminwel_tag *));
struct TSetc_minproj_tag *DestroyTSetc_minproj(struct TSetc_minproj_tag *);
//And creates the following user's function.
//static double minneglogpost(struct TSminpack_tag *minpack_ps);  //For the constant-parameter only.
//=== Step 2. Belongs to the user's responsibility because this function must be able to deal with
//               (1) constant-parameter case without using DW's functions;
//               (2) allowing us to generate parameters randomly, which depends on the specific model.
//           See lwz_est.c in D:\ZhaData\WorkDisk\LiuWZ\Project2_empirical\EstimationOct07
//            or ExamplesForC.prn in D:\ZhaData\CommonFiles\C_Examples_DebugTips.
//---
//void InitializeForMinproblem(struct TSminpack_tag *minpack_ps, char *filename_sp, TSdvector *gphi_dv, int indxStartValuesForMin);
//=== Step 3.
void minfinder_blockcsminwel(struct TSminpack_tag *minpack_ps, int indx_findMLE); //Blockwise minimization.
                                 //indx_findMLE: 1: find MLE without a prior, 0: find posterior (with a prior).




//================ For IMSL multivariate linearly-constrained minimizaiton package only. ================//
typedef struct TSpackage_imslconlin_tag
{
   //=== Non-simple constraints.
   int npars_tot; //Total number of free parameters for the optimaization.
                  //For example, model free parameters + free transition matrix parameters in the regime-switching case.
   int neqs;  //Number of equality constraints, excluding simple bound constraints. Must be no greater than ncons.
              //IMSL dictates that equality constraints come always BEFORE inequality constrains.
   int ncons; //Total number of constrains, including equality and inequality constraints, but excluding simple bounds.
   TSdvector *lh_coefs_dv;  //ncons*npars_tot-by-1. ALWAYS initialized to be 0.0.
                            //Left-hand coefficients in the linear constrains (excluding simple bounds).
                            //IMSL rule: lh_coefs_dv stacks the neqs rows of equality constraints first, followed by the inequality constraints.
                            //Set to NULL if ncons=0;
   TSdvector *rh_constraints_dv;  //ncons-by-1.  Set to NULL if ncons=0.
                                  //Right-hand constraints in the equality and non-equality constrains (excluding simple bounds).


   //=== Simple bounds.
   TSdvector *lowbounds_dv; //npars_tot-by-1.  ALWAYS initialized to -BIGREALNUMBER for thes simple lower bounds.
                            //If a component is unbounded, choose a very negative large value (e.g., -BIGREALNUMBER).
   TSdvector *upperbounds_dv; //npars_tot-by-1.  ALWAYS initialized to +BIGREALNUMBER for thes simple lower bounds.
                              //If a component is unbounded, choose a very positive large value (e.g., BIGREALNUMBER).

   //=== Other output.
   TSdvector *xsaved_dv; //npars_tot-by-1. Saved the parameters that give the minimal value of the objective function.

   //=== Other inputs.
   double crit;   //Overall convergence criterion on the first-order conditions.
   int itmax;     //Maximum number of iterations.
} TSpackage_imslconlin;
//+
struct TSpackage_imslconlin_tag *CreateTSpackagae_imslconlin(const int npars_tot, const int neqs, const int ncons);
struct TSpackage_imslconlin_tag *DestroyTSpackagae_imslconlin(struct TSpackage_imslconlin_tag *package_imslconlin_ps);
void minfinder_noblockimslconlin(struct TSpackage_imslconlin_tag *package_imslconlin_ps, struct TSminpack_tag *minpack_ps, char *filename_printout, int ntheta);





//================ For conjugate gradient method I only. ================//
typedef struct TSpackage_congrad1_tag
{
   //=== Input arguments.
   double crit;   //Overall convergence criterion on the function value.
   int itmax;     //Maximum number of iterations.

   //=== Output arguments.
   int niters;   //Number of iterations.
} TSpackage_congrad1;
//+
struct TSpackage_congrad1_tag *CreateTSpackage_congrad1(void);
struct TSpackage_congrad1_tag *DestroyTSpackage_congrad1(struct TSpackage_congrad1_tag *package_congrad1_ps);




/**
//------- For unconstrained BFGS csminwel only. -------
typedef struct TSminpack_csminwel_tag {
   //=== Optional input arguments, often NOT used, so we set to NULL at this point.
   double **args; //k-by-q.
   int *dims;   //k-by-1;
   int _k;

   //=== Mandatory input arguments.
   TSdmatrix *Hx_dm;  //n-by-n inverse Hessian.  Output as well, when csminwel is done.
   double crit;   //Overall convergence criterion for the function value.
   int itmax;  //Maximum number of iterations.
//   double grdh;  //Step size for the numerical gradient if no analytical gradient is available.

   //=== Initial input arguments.
   double ini_h_csminwel;
   int indxnumgrad_csminwel;
   double gradstps_csminwel;


   //=== Output arguments.
   int badg;    //If (badg==0), analytical gradient is used; otherwise, numerical gradient will be produced.
   int niter;   //Number of iterations taken by csminwel.
   int fcount;   //Number of function evaluations used by csminwel.
   int retcode;  //Return code for the terminating condition.
                // 0, normal step (converged). 1, zero gradient (converged).
                // 4,2, back and forth adjustment of stepsize didn't finish.
                // 3, smallest stepsize still improves too slow. 5, largest step still improves too fast.
                // 6, no improvement found.
} TSminpack_csminwel;
/**/



#endif


/*************** 3 steps to find minimization solution.  *****************
//---------------------------------
//-- For concrete examples, see
//--  lwz_est.c in D:\ZhaData\WorkDisk\LiuWZ\Project2_empirical\EstimationOct07
//--  ExamplesForC.prn in D:\ZhaData\CommonFiles\C_Examples_DebugTips
//---------------------------------

//------ For the csminwel minimization problem. -------
//--- Step 1. Creats a number of csminwel structures for both Markov-switching and constant-parameter models.
static double minobj(struct TSminpack_tag *minpack_ps);  //This function is for the constant-parameter model only.
//--- Step 2.
static void InitializeForMinproblem(struct TSminpack_tag *minpack_ps, char *filename_sp);
//--- Step 3.
//For the constant-parameter model, run minfinder(minpack_ps);  //Constant-parameter case.
//For the regime-switching model, run minfinder_blockcsminwel(minpack_ps);  //Time-varying case.
 *
 *
 *


//=== main.c

int indxInitializeTransitionMatrix;
//--- My model structure.
struct TSlwzmodel_tag *lwzmodel_ps = NULL;
//--- Waggoner's Markov switching package.
struct TMarkovStateVariable_tag *sv_ps = NULL;
ThetaRoutines *sroutines_ps = NULL;
struct TStateModel_tag *smodel_ps = NULL;
//--- General (csminwel) minimization for constant-parameter.
struct TSetc_minproj_tag *etc_minproj_ps = NULL;
struct TSminpack_tag *minpack_ps = NULL;
//--- Blockwise (csminwel) minimization for regime-switching model.
struct TSargs_blockcsminwel_tag *args_blockcsminwel_ps = NULL;


//-----------------
// Reads from the command line the user-specified input file and the most-often-used integer arguments such as sample size.
//-----------------
cl_modeltag = fn_ParseCommandLine_String(n_arg,args_cl,'t',(char *)NULL);  // Tag for different models.
if (!cl_modeltag)  fn_DisplayError(".../main(): No model tag is specified yet");
//--- Type of the model: (0) const, (1) varionly, (2) trendinf, (3) policyonly, and (4) firmspolicy.
if (!strncmp("const", cl_modeltag, 5))  indx_tvmodel = 0;
else if  (!strncmp("varionly", cl_modeltag, 5)) indx_tvmodel = 1;
else if  (!strncmp("trendinf", cl_modeltag, 5)) indx_tvmodel = 2;
else if  (!strncmp("policyonly", cl_modeltag, 5)) indx_tvmodel = 3;
else if  (!strncmp("firmspolicy", cl_modeltag, 5)) indx_tvmodel = 4;
else  fn_DisplayError("main(): the model tag is NOT properly selected");
indxStartValuesForMin = fn_ParseCommandLine_Integer(n_arg,args_cl,'c',1);
sprintf(filename_sp_vec_minproj, "outdatasp_min_%s.prn", cl_modeltag);
//+
sprintf(filenamebuffer, "dataraw.prn");
cl_filename_rawdata = fn_ParseCommandLine_String(n_arg,args_cl,'r',filenamebuffer);  //Raw data input file.
fptr_rawdata = tzFopen(cl_filename_rawdata,"r");
//+
sprintf(filenamebuffer, "datainp_common.prn");
cl_filename_input1 = fn_ParseCommandLine_String(n_arg,args_cl,'i',filenamebuffer);  //Common setup input data file.
fptr_input1 = tzFopen(cl_filename_input1,"r");
//+
sprintf(filenamebuffer, "datainp_%s.prn", cl_modeltag);
cl_filename_input2 = fn_ParseCommandLine_String(n_arg,args_cl,'s',filenamebuffer);  //Model-specific setupt input data file.
fptr_input2 = tzFopen(cl_filename_input2,"r");
//+
sprintf(filenamebuffer, "datainp_markov_%s.prn", cl_modeltag);
cl_filename_markov = fn_ParseCommandLine_String(n_arg,args_cl,'m',filenamebuffer);  //Markov-switching setup input data file.
fptr_markov = tzFopen(cl_filename_markov,"r");
//--- Output data files.
sprintf(filenamebuffer, "outdata_debug_%s.prn", cl_modeltag);
FPTR_DEBUG = tzFopen(filenamebuffer,"w");  //Debug output file.
//+
sprintf(filenamebuffer, "outdataout_%s.prn", cl_modeltag);
fptr_output = tzFopen(filenamebuffer,"w");  //Final output file.
//+
sprintf(filenamebuffer, "outdatainp_matlab_%s.prn", cl_modeltag);
fptr_matlab = tzFopen(filenamebuffer, "w");
//+
sprintf(filenamebuffer, "outdatainp_matlab1_%s.prn", cl_modeltag);
fptr_matlab1 = tzFopen(filenamebuffer, "w");
//+
sprintf(filenamebuffer, "outdatainp_matlab2_%s.prn", cl_modeltag);
fptr_matlab2 = tzFopen(filenamebuffer, "w");
//+
sprintf(filenamebuffer, "outdatainp_matlab3_%s.prn", cl_modeltag);
fptr_matlab3 = tzFopen(filenamebuffer, "w");


//----------------------------------------------
//--- Memory allocation and structure creation.
//--- The order matters!
//----------------------------------------------
//--- Model structure. ---
lwzmodel_ps = CreateTSlwzmodel(fptr_rawdata, fptr_input1, fptr_input2, fptr_markov, indx_tvmodel, indxStartValuesForMin);
sprintf(lwzmodel_ps->tag_modeltype_cv->v, cl_modeltag);
lwzmodel_ps->tag_modeltype_cv->flag = V_DEF;


//====== Waggoner's Markov switching variables. ======
sv_ps = CreateMarkovStateVariable_File(fptr_markov, (char *)NULL, lwzmodel_ps->fss);
         //In this case, fptr_markov points to datainp_markov_const.prn, which can be found in D:\ZhaData\CommonFiles\C_Examples_DebugTips\DW_MarkovInputFiles.
sroutines_ps = CreateThetaRoutines_empty();
sroutines_ps->pLogConditionalLikelihood = logTimetCondLH;                                     //User's: logTimetCondLH
sroutines_ps->pLogPrior = logpriordensity_usingDW;                                            //User's: pLogPrior
sroutines_ps->pNumberFreeParametersTheta = NumberOfFreeModelSpecificParameters;               //User's: NumberOfFreeModelSpecificParameters,
sroutines_ps->pConvertFreeParametersToTheta = ConvertFreeParameters2ModelSpecificParameters;  //User's: ConvertFreeParameters2ModelSpecificParameters,
sroutines_ps->pConvertThetaToFreeParameters = ConvertModelSpecificParameters2FreeParameters;  //User's: ConvertModelSpecificParameters2FreeParameters,
sroutines_ps->pThetaChanged = tz_thetaChanged;                                                   //User's: Notification routine (need to refresh everything given new parameters?)
sroutines_ps->pTransitionMatrixChanged = tz_TransitionMatrixChanged;                             //User's: Notification routine (need to refresh everything given new parameters?)
smodel_ps = CreateStateModel_new(sv_ps, sroutines_ps, (void *)lwzmodel_ps);
//--- Optional.
if (!indx_tvmodel && fn_SetFilePosition(fptr_markov, "//== indxInitializeTransitionMatrix ==//"))
   if ((fscanf(fptr_markov, " %d ", &indxInitializeTransitionMatrix) == 1) && indxInitializeTransitionMatrix)
      ReadTransitionMatrices(fptr_markov, (char*)NULL, "Initial: ", smodel_ps);  //Waggoner's function.


//--- Minimization problem: Step 1. ---
args_blockcsminwel_ps = CreateTSargs_blockcsminwel(fptr_input1);
             //Blockwise (csminwel) minimization arguments, reading convergence criteria or using default values if fptr_input1 is set to NULL.
             //fptr_input1 contains parameters for both constant-parameter and Markov-switching models.
etc_minproj_ps = CreateTSetc_minproj(&smodel_ps, (TFDestroyTStateModel *)NULL, &args_blockcsminwel_ps, DestroyTSargs_blockcsminwel);
                 //Taking convergence criteria and my model structure smodel_ps into minpack_ps.
minpack_ps = CreateTSminpack((TFminobj *)minobj, (void **)&etc_minproj_ps, (TFmindestroy_etcproject *)NULL, (TFmingrad *)NULL,
                             filename_sp_vec_minproj,
                             lwzmodel_ps->xphi_dv->n+NumberFreeParametersQ(smodel_ps),
                             MIN_CSMINWEL);
          //minobj is for the constant-parameter model only in which case, NumberFreeParametersQ(smodel_ps) will be 0.


//-----------------
// Main matter.
//-----------------
time(&lwzmodel_ps->prog_begtime);  //Beginning time of the whole program.
InitializeGlobleSeed(lwzmodel_ps->randomseed = 0);  //2764 If seednumber==0, a value is computed using the system clock.
csminwel_randomseedChanged(lwzmodel_ps->randomseed);  //Using the same (or different) seednumber to reset csminwel seednumber for random perturbation.
//=== Finding the peak value of the logLH or logPosterior
if (lwzmodel_ps->indxEstFinder)
{
   //Minimization problem: Steps 2 and 3.

   InitializeForMinproblem(minpack_ps, filename_sp_vec_minproj); //Initialization for minimization.
   //======= 1st round of minimization. =======
   //--------------------------
   //-- csminwel minimization where
   //--  minpack_ps->x_dv contains the minimizing vector of parameters.
   //--  minpack_ps->fret contains the minimized value.
   //--------------------------

   if (!lwzmodel_ps->indx_tvmodel)  minfinder(minpack_ps);  //Constant-parameter case.
   else  minfinder_blockcsminwel(minpack_ps);  //Time-varying case.
}
else  InitializeForMinproblem(minpack_ps, filename_sp_vec_minproj);
time(&lwzmodel_ps->prog_endtime);  //Ending time of the whole program.
/*************************************************************************/

