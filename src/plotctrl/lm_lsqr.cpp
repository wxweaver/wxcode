/////////////////////////////////////////////////////////////////////////////
// Name:        lm_lsqr.cpp
// Purpose:     Levenberg-Marquart nonlinear least squares 2-D curve fitting
// Author:      John Labenski, mostly others (see lm_lsqr.h)
// Modified by:
// Created:     6/5/2002
// Copyright:   (c) John Labenski, mostly others (see lm_lsqr.h)
// Licence:     Public domain
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma implementation "lm_lsqr.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/log.h"
#endif // WX_PRECOMP

#include "wx/math.h"

#include "wxcode/plotctrl/lm_lsqr.h"
#include "wxcode/plotctrl/plotdata.h"

#undef BUG
#define BUG 0

//----------------------------------------------------------------------------
// LM_LeastSquareProgressHandler_
//----------------------------------------------------------------------------

static LM_LeastSquareProgressHandler_ s_lm_leastsquarehandler = (LM_LeastSquareProgressHandler_)NULL;

void SetLM_LeastSquareProgressHandler( LM_LeastSquareProgressHandler_ handler )
{
    s_lm_leastsquarehandler = handler;
}

static int s_nprint = 0;

void SetLM_LeastSquareProgressHandlerTicks( int iterations )
{
    wxCHECK_RET( (iterations>=0), wxT("invalid iteration step size"));
    s_nprint = iterations;
}

/************************lmmisc.c*************************/

inline double dmax1(double a, double b) { return ( a >= b ) ? a : b; }
inline double dmin1(double a, double b) { return ( a <= b ) ? a : b; }

inline int min0(int a, int b)  { return ( a <= b ) ? a : b; }
inline int mod( int k, int m ) { return ( k % m ); }

void pmat( int m, int n, double y[]  )
{
    int i, j, k = 0;

    for( i=0; i<m; i++ )
    {
        for( j=0; j<n; j++ )
        {
            printf( "%.5e ", y[k] );
            k += 1;
        }
        printf( "\n" );
    }
    fflush(stdout);
}

//=============================================================================
// LM_LeastSquare
//=============================================================================
LM_LeastSquare::LM_LeastSquare()
{
    Init();
}

void LM_LeastSquare::Init()
{
    ReInit();

    m_init_value = 0.1;

    m_eps    = 1.2e-16;
    m_dwarf  = 1.0e-38;
    m_ftol   = 1.0e-14;
    m_xtol   = 1.0e-14;
    m_gtol   = 1.0e-14;
    m_epsfcn = 1.0e-15;
    m_factor = 0.1;
}

void LM_LeastSquare::ReInit()
{
    m_plotData = NULL;
    m_plotFunc = NULL;
    m_resultMsg.Clear();
    m_ok = false;
    m_fitting = false;
    m_abort_fitting = false;

    m_vars = NULL;
    m_x    = NULL;
    m_fvec = NULL;
    m_diag = NULL;
    m_fjac = NULL;
    m_qtf  = NULL;
    m_ipvt = NULL;

    m_nan   = 0;
    m_n     = 0;
    m_m     = 0;
    m_info  = -1;
    m_fnorm = 0;
    m_mode  = 1;

    m_nfev   = 0;
    m_ldfjac = 0;
}

void LM_LeastSquare::Destroy()
{
    wxCHECK_RET(!IsFitting(), wxT("Cannot Destroy Least Square when currently fitting"));
    if (m_plotData) delete m_plotData;
    if (m_plotFunc) delete m_plotFunc;

    if (m_vars) free(m_vars);
    if (m_x)    free(m_x);
    if (m_fvec) free(m_fvec);
    if (m_diag) free(m_diag);
    if (m_fjac) free(m_fjac);
    if (m_qtf)  free(m_qtf);
    if (m_ipvt) free(m_ipvt);

    ReInit();
}

bool LM_LeastSquare::Create(const wxPlotData &plotData, const wxPlotFunction &plotFunc)
{
    wxCHECK_MSG(!IsFitting(), false, wxT("Cannot recreate Least Square when currently fitting"));
    Destroy();

    wxCHECK_MSG(plotData.Ok() && plotFunc.Ok(), false, wxT("invalid functions"));
    wxCHECK_MSG((plotData.GetCount() > 1) && (plotData.GetCount() >= plotFunc.GetNumberVariables() - 1), false, wxT("invalid dimensions"));

    m_plotData = new wxPlotData(plotData);
    m_plotFunc = new wxPlotFunction(plotFunc);

    double *x_data = m_plotData->GetXData();
    double *y_data = m_plotData->GetYData();
    int data_count = m_plotData->GetCount();
    for (int i = 0; i < data_count; i++, x_data++, y_data++)
    {
        if (!wxFinite(*x_data) && !wxFinite(*y_data))
        {
            Destroy();
            m_resultMsg.Printf(wxT("Unable to fit data since some values are NaN"));
            return false;
        }
    }

    m_n = plotFunc.GetNumberVariables() - 1;
    m_m = plotData.GetCount();

    m_ldfjac = m_m;

    m_vars = (double*)malloc((m_n+1) * sizeof(double));
    m_x    = (double*)malloc(m_n * sizeof(double));
    m_fvec = (double*)malloc(m_m * sizeof(double));
    m_diag = (double*)malloc(m_n * sizeof(double));
    m_fjac = (double*)malloc(m_m * m_n * sizeof(double));
    m_qtf  = (double*)malloc(m_n * sizeof(double));
    m_ipvt = (int*)malloc(m_n * sizeof(int));
    m_maxfev = 200 * (m_n + 1);

    if (!(m_plotData && m_plotFunc && m_vars && m_x && m_fvec && m_diag && m_fjac && m_qtf && m_ipvt))
    {
        Destroy();
        wxFAIL_MSG(wxT("can't allocate memory for LM_LeastSquare::Create"));
        return false;
    }

    memset(m_x, 0, m_n * sizeof(double));

    m_ok = true;
    return true;
}

int LM_LeastSquare::Fit(const double *x0, int init_count)
{
    wxCHECK_MSG(Ok() && !IsFitting(), 0, wxT("invalid functions"));

    m_nan   = 0;
    m_info  = -1;
    m_fnorm = 0;
    m_nfev  = 0;

    int i;

    if (x0 && (init_count > 0))
    {
        wxCHECK_MSG(init_count <= m_n, 0, wxT("Invalid initializer count"));

        // initialize the variables
        for (i = 0; i < init_count; i++)
        {
            if (!wxFinite(x0[i]))
            {
                m_x[i] = m_init_value;
                wxLogWarning(wxT("Initial value is NaN in LM_LeastSquare::Fit"));
            }
            else
                m_x[i] = x0[i];
        }

        // initialize the rest, if any
        for (i = init_count; i < m_n; i++)
            m_x[i] = m_init_value;
    }
    else
    {
        for (i = 0; i < m_n; i++)
            m_x[i] = m_init_value;
    }

    double *wa1 = (double*)malloc(m_n * sizeof(double));
    double *wa2 = (double*)malloc(m_n * sizeof(double));
    double *wa3 = (double*)malloc(m_n * sizeof(double));
    double *wa4 = (double*)malloc(m_m * sizeof(double));

    if (!(wa1 && wa2 && wa3 && wa4))
    {
        if (wa1) free(wa1);
        if (wa2) free(wa2);
        if (wa3) free(wa3);
        if (wa4) free(wa4);

        wxFAIL_MSG(wxT("can't allocate memory for LM_LeastSquare::Fit"));
        return 0;
    }

    m_fitting = true;        // start fitting here, just before it starts
    m_abort_fitting = false; // reset abort flag

#if BUG
    int iflag = 1;
    fcn(m_m, m_n, m_x, m_fvec, &iflag);
    printf( "initial x\n" ); pmat( 1, m_n, m_x ); // display 1 by n matrix
    //printf( "initial function\n" ); pmat( 1, m, fvec );
#endif

    lmdif(m_m, m_n, m_x, m_fvec, m_ftol, m_xtol, m_gtol, m_maxfev, m_epsfcn,
          m_diag, m_mode, m_factor, s_nprint, &m_info, &m_nfev, m_fjac,
          m_ldfjac, m_ipvt, m_qtf, wa1, wa2, wa3, wa4);

    m_fnorm = enorm(m_m, m_fvec);

#if BUG
    printf( "%d function evaluations\n", m_nfev );
    // display solution and function vector

    //printf( "fvec\n" ); pmat( 1, m_m, m_fvec );
    printf( "x\n" );
    pmat( 1, m_n, m_x );
    printf( "function norm = %.15e\n", m_fnorm );
    // display m_info returned by lmdif
    printf( "%s\n", GetResultMessage().c_str() );
#endif

    if (wa1) free(wa1);
    if (wa2) free(wa2);
    if (wa3) free(wa3);
    if (wa4) free(wa4);

    m_fitting = false;
    return m_nfev;
}

wxString LM_LeastSquare::GetResultMessage() const
{
    wxString msg;

    switch (m_info)
    {
        case -1 : return m_resultMsg; // error before fitting
        case  0 : msg.Printf(wxT("Improper input parameters.")); break;
        case  1 : msg.Printf(wxT("Both actual and predicted relative reductions in the sum of squares are at most ftol (%lg)."), m_ftol); break;
        case  2 : msg.Printf(wxT("Relative error between two consecutive iterates is at most xtol (%lg)."), m_xtol); break;
        case  3 : msg.Printf(wxT("Both actual and predicted relative reductions in the sum of squares are at most ftol (%lg)."), m_ftol);
                 msg += wxString::Format(wxT("Relative error between two consecutive iterates is at most xtol (%lg)."), m_xtol); break;
        case  4 : msg.Printf(wxT("The cosine of the angle between fvec and any column of the jacobian is at most gtol (%lg) in absolute value."), m_gtol); break;
        case  5 : msg.Printf(wxT("Number of iterations has reached or exceeded %d, try adjusting initial values."), m_maxfev); break;
        case  6 : msg.Printf(wxT("ftol (%lg) is too small. no further reduction in the sum of squares is possible"), m_ftol); break;
        case  7 : msg.Printf(wxT("xtol (%lg) too small. no further improvement in approximate solution x possible"), m_xtol); break;
        case  8 : msg.Printf(wxT("gtol (%lg) is too small. fvec is orthogonal to the columns of the jacobian to machine precision."), m_gtol); break;
        default : break;
    }

    if (m_nan)
        msg += wxString::Format(wxT(" %ld values were skipped since they were NaN"), m_nan);

    return msg;
}

double LM_LeastSquare::GetVariable(int n)
{
    wxCHECK_MSG(m_x, 0.0, wxT("LM_LeastSquare not created"));
    wxCHECK_MSG((n>=0)&&(n<m_n), 0.0, wxT("Invalid variable index"));
    return m_x[n];
}

/*
*   fcn is the name of the user-supplied subroutine which
*     calculates the functions. fcn must be declared
*     in an external statement in the user calling
*     program, and should be written as follows.
*
*     subroutine fcn(m,n,x,fvec,iflag)
*     integer m,n,iflag
*     double precision x(n),fvec(m)
*     ----------
*     calculate the functions at x and
*     return this vector in fvec.
*     ----------
*     return
*     end
*
*     the value of iflag should not be changed by fcn unless
*     the user wants to terminate execution of lmdif1.
*     in this case set iflag to a negative integer.
*
*   m is a positive integer input variable set to the number
*     of functions.
*
*   n is a positive integer input variable set to the number
*     of variables. n must not exceed m.
*
*   x is an array of length n. on input x must contain
*     an initial estimate of the solution vector. on output x
*     contains the final estimate of the solution vector.
*
*   fvec is an output array of length m which contains
*     the functions evaluated at the output x.
*
*
*     argonne national laboratory. minpack project. march 1980.
*     burton s. garbow, kenneth e. hillstrom, jorge j. more
*
*     **********
*/

void LM_LeastSquare::fcn(int m, int n, double x[], double fvec[], int *iflag)
{
    wxCHECK_RET(m_plotData && m_plotFunc, wxT("invalid functions"));

    if (*iflag == 0)
    {
        if (m_abort_fitting)
            *iflag = -1;
        else if (s_lm_leastsquarehandler)
        {
            // exit if handler returns false
            if (!((*s_lm_leastsquarehandler)(m_plotFunc->GetFunctionString(), m_nfev, m_maxfev)))
                *iflag = -1;
        }

        return;
    }

    double *x_data = m_plotData->GetXData();
    double *y_data = m_plotData->GetYData();
    double f = 0;
    memcpy(m_vars, x, n*sizeof(double));

    for (int i = 0; i < m; i++, x_data++, y_data++)
    {
/*      // this is checked in Create now
        if (!wxFinite(*x_data) && !wxFinite(*y_data))
        {
            m_nan++;
            fvec[i] = 0;
            continue;
        }
*/
        m_vars[n] = *x_data;

        f = m_plotFunc->GetValue(m_vars);
        if (!wxFinite(f))
        {
            m_nan++;
            fvec[i] = 0;
            continue;
        }

        fvec[i] = (*y_data) - f;
    }
}

/*********************** lmdif.c ****************************/
#define BUG 0

/*
*     **********
*
*     subroutine lmdif
*
*     the purpose of lmdif is to minimize the sum of the squares of
*     m nonlinear functions in n variables by a modification of
*     the levenberg-marquardt algorithm. the user must provide a
*     subroutine which calculates the functions. the jacobian is
*     then calculated by a forward-difference approximation.
*
*     the subroutine statement is
*
*   subroutine lmdif(fcn,m,n,x,fvec,ftol,xtol,gtol,maxfev,epsfcn,
*            diag,mode,factor,nprint,info,nfev,fjac,
*            ldfjac,ipvt,qtf,wa1,wa2,wa3,wa4)
*
*     where
*
*   fcn is the name of the user-supplied subroutine which
*     calculates the functions. fcn must be declared
*     in an external statement in the user calling
*     program, and should be written as follows.
*
*     subroutine fcn(m,n,x,fvec,iflag)
*     integer m,n,iflag
*     double precision x(n),fvec(m)
*     ----------
*     calculate the functions at x and
*     return this vector in fvec.
*     ----------
*     return
*     end
*
*     the value of iflag should not be changed by fcn unless
*     the user wants to terminate execution of lmdif.
*     in this case set iflag to a negative integer.
*
*   m is a positive integer input variable set to the number
*     of functions.
*
*   n is a positive integer input variable set to the number
*     of variables. n must not exceed m.
*
*   x is an array of length n. on input x must contain
*     an initial estimate of the solution vector. on output x
*     contains the final estimate of the solution vector.
*
*   fvec is an output array of length m which contains
*     the functions evaluated at the output x.
*
*   ftol is a nonnegative input variable. termination
*     occurs when both the actual and predicted relative
*     reductions in the sum of squares are at most ftol.
*     therefore, ftol measures the relative error desired
*     in the sum of squares.
*
*   xtol is a nonnegative input variable. termination
*     occurs when the relative error between two consecutive
*     iterates is at most xtol. therefore, xtol measures the
*     relative error desired in the approximate solution.
*
*   gtol is a nonnegative input variable. termination
*     occurs when the cosine of the angle between fvec and
*     any column of the jacobian is at most gtol in absolute
*     value. therefore, gtol measures the orthogonality
*     desired between the function vector and the columns
*     of the jacobian.
*
*   maxfev is a positive integer input variable. termination
*     occurs when the number of calls to fcn is at least
*     maxfev by the end of an iteration.
*
*   epsfcn is an input variable used in determining a suitable
*     step length for the forward-difference approximation. this
*     approximation assumes that the relative errors in the
*     functions are of the order of epsfcn. if epsfcn is less
*     than the machine precision, it is assumed that the relative
*     errors in the functions are of the order of the machine
*     precision.
*
*   diag is an array of length n. if mode = 1 (see
*     below), diag is internally set. if mode = 2, diag
*     must contain positive entries that serve as
*     multiplicative scale factors for the variables.
*
*   mode is an integer input variable. if mode = 1, the
*     variables will be scaled internally. if mode = 2,
*     the scaling is specified by the input diag. other
*     values of mode are equivalent to mode = 1.
*
*   factor is a positive input variable used in determining the
*     initial step bound. this bound is set to the product of
*     factor and the euclidean norm of diag*x if nonzero, or else
*     to factor itself. in most cases factor should lie in the
*     interval (.1,100.). 100. is a generally recommended value.
*
*   nprint is an integer input variable that enables controlled
*     printing of iterates if it is positive. in this case,
*     fcn is called with iflag = 0 at the beginning of the first
*     iteration and every nprint iterations thereafter and
*     immediately prior to return, with x and fvec available
*     for printing. if nprint is not positive, no special calls
*     of fcn with iflag = 0 are made.
*
*   info is an integer output variable. if the user has
*     terminated execution, info is set to the (negative)
*     value of iflag. see description of fcn. otherwise,
*     info is set as follows.
*
*     info = 0  improper input parameters.
*
*     info = 1  both actual and predicted relative reductions
*           in the sum of squares are at most ftol.
*
*     info = 2  relative error between two consecutive iterates
*           is at most xtol.
*
*     info = 3  conditions for info = 1 and info = 2 both hold.
*
*     info = 4  the cosine of the angle between fvec and any
*           column of the jacobian is at most gtol in
*           absolute value.
*
*     info = 5  number of calls to fcn has reached or
*           exceeded maxfev.
*
*     info = 6  ftol is too small. no further reduction in
*           the sum of squares is possible.
*
*     info = 7  xtol is too small. no further improvement in
*           the approximate solution x is possible.
*
*     info = 8  gtol is too small. fvec is orthogonal to the
*           columns of the jacobian to machine precision.
*
*   nfev is an integer output variable set to the number of
*     calls to fcn.
*
*   fjac is an output m by n array. the upper n by n submatrix
*     of fjac contains an upper triangular matrix r with
*     diagonal elements of nonincreasing magnitude such that
*
*        t     t           t
*       p *(jac *jac)*p = r *r,
*
*     where p is a permutation matrix and jac is the final
*     calculated jacobian. column j of p is column ipvt(j)
*     (see below) of the identity matrix. the lower trapezoidal
*     part of fjac contains information generated during
*     the computation of r.
*
*   ldfjac is a positive integer input variable not less than m
*     which specifies the leading dimension of the array fjac.
*
*   ipvt is an integer output array of length n. ipvt
*     defines a permutation matrix p such that jac*p = q*r,
*     where jac is the final calculated jacobian, q is
*     orthogonal (not stored), and r is upper triangular
*     with diagonal elements of nonincreasing magnitude.
*     column j of p is column ipvt(j) of the identity matrix.
*
*   qtf is an output array of length n which contains
*     the first n elements of the vector (q transpose)*fvec.
*
*   wa1, wa2, and wa3 are work arrays of length n.
*
*   wa4 is a work array of length m.
*
*     subprograms called
*
*   user-supplied ...... fcn
*
*   minpack-supplied ... dpmpar,enorm,fdjac2,lmpar,qrfac
*
*   fortran-supplied ... dabs,dmax1,dmin1,dsqrt,mod
*
*     argonne national laboratory. minpack project. march 1980.
*     burton s. garbow, kenneth e. hillstrom, jorge j. more
*
*     **********
*/

void LM_LeastSquare::lmdif( int m, int n, double x[], double fvec[],
                            double ftol, double xtol, double gtol, int maxfev,
                            double epsfcn, double diag[], int mode,
                            double factor, int nprint, int *info, int *nfev,
                            double fjac[], int ldfjac, int ipvt[], double qtf[],
                            double wa1[], double wa2[], double wa3[], double wa4[])
{
    int i, iflag, ij, jj, iter, j, l;
    double actred, delta, dirder, fnorm, fnorm1, gnorm;
    double par, pnorm, prered, ratio;
    double sum, temp, temp1, temp2, temp3, xnorm;
    static double one = 1.0;
    static double p1 = 0.1;
    static double p5 = 0.5;
    static double p25 = 0.25;
    static double p75 = 0.75;
    static double p0001 = 1.0e-4;
    static double zero = 0.0;
    // static double p05 = 0.05;

    *info = 0;
    iflag = 0;
    *nfev = 0;
    delta = 0.0;
    xnorm = 0.0;

    // check the input parameters for errors.

    if ( (n <= 0) || (m < n) || (ldfjac < m) || (ftol < zero)
        || (xtol < zero) || (gtol < zero) || (maxfev <= 0) || (factor <= zero) )
        goto L300;

    if( mode == 2 )
    {
        // scaling by diag[]
        for( j=0; j<n; j++ )
        {
            if( diag[j] <= 0.0 )
                goto L300;
        }
    }
#if BUG
    printf( "lmdif\n" );
#endif

    // evaluate the function at the starting point
    // and calculate its norm.

    iflag = 1;
    fcn(m,n,x,fvec,&iflag);
    *nfev = 1;
    if(iflag < 0)
        goto L300;
    fnorm = enorm(m,fvec);

    // initialize levenberg-marquardt parameter and iteration counter.

    par = zero;
    iter = 1;

    // beginning of the outer loop.

L30:

    // calculate the jacobian matrix.

    iflag = 2;
    fdjac2(m,n,x,fvec,fjac,ldfjac,&iflag,epsfcn,wa4);
    *nfev += n;
    if(iflag < 0)
        goto L300;

    // if requested, call fcn to enable printing of iterates.

    if( nprint > 0 )
    {
        iflag = 0;
        if(mod(iter-1,nprint) == 0)
        {
            fcn(m,n,x,fvec,&iflag);
            if(iflag < 0)
                goto L300;
#if BUG
            printf( "fnorm %.15e\n", enorm(m,fvec) );
#endif
        }
    }

    // compute the qr factorization of the jacobian.

    qrfac(m,n,fjac,ldfjac,1,ipvt,n,wa1,wa2,wa3);

    // on the first iteration and if mode is 1, scale according
    // to the norms of the columns of the initial jacobian.

    if(iter == 1)
    {
        if(mode != 2)
        {
            for( j=0; j<n; j++ )
            {
                diag[j] = wa2[j];
                if( wa2[j] == zero )
                    diag[j] = one;
            }
        }

        // on the first iteration, calculate the norm of the scaled x
        // and initialize the step bound delta.

        for( j=0; j<n; j++ )
            wa3[j] = diag[j] * x[j];

        xnorm = enorm(n,wa3);
        delta = factor*xnorm;
        if(delta == zero)
            delta = factor;
    }

    // form (q transpose)*fvec and store the first n components in qtf.

    for( i=0; i<m; i++ )
        wa4[i] = fvec[i];

    jj = 0;
    for( j=0; j<n; j++ )
    {
        temp3 = fjac[jj];
        if(temp3 != zero)
        {
            sum = zero;
            ij = jj;
            for( i=j; i<m; i++ )
            {
                sum += fjac[ij] * wa4[i];
                ij += 1;    // fjac[i+m*j]
            }
            temp = -sum / temp3;
            ij = jj;
            for( i=j; i<m; i++ )
            {
                wa4[i] += fjac[ij] * temp;
                ij += 1;    // fjac[i+m*j]
            }
        }
        fjac[jj] = wa1[j];
        jj += m+1;  // fjac[j+m*j]
        qtf[j] = wa4[j];
    }

    // compute the norm of the scaled gradient.

    gnorm = zero;
    if(fnorm != zero)
    {
        jj = 0;
        for( j=0; j<n; j++ )
        {
            l = ipvt[j];
            if(wa2[l] != zero)
            {
                sum = zero;
                ij = jj;
                for( i=0; i<=j; i++ )
                {
                    sum += fjac[ij]*(qtf[i]/fnorm);
                    ij += 1; // fjac[i+m*j]
                }
                gnorm = dmax1(gnorm,fabs(sum/wa2[l]));
            }
            jj += m;
        }
    }

    // test for convergence of the gradient norm.

    if(gnorm <= gtol)
        *info = 4;
    if( *info != 0)
        goto L300;

    // rescale if necessary.

    if(mode != 2)
    {
        for( j=0; j<n; j++ )
            diag[j] = dmax1(diag[j],wa2[j]);
    }

    // beginning of the inner loop.

L200:

    // determine the levenberg-marquardt parameter.

    lmpar(n,fjac,ldfjac,ipvt,diag,qtf,delta,&par,wa1,wa2,wa3,wa4);

    // store the direction p and x + p. calculate the norm of p.

    for( j=0; j<n; j++ )
    {
        wa1[j] = -wa1[j];
        wa2[j] = x[j] + wa1[j];
        wa3[j] = diag[j]*wa1[j];
    }
    pnorm = enorm(n,wa3);

    // on the first iteration, adjust the initial step bound.

    if(iter == 1)
        delta = dmin1(delta,pnorm);

    // evaluate the function at x + p and calculate its norm.

    iflag = 1;
    fcn(m,n,wa2,wa4,&iflag);
    *nfev += 1;
    if(iflag < 0)
        goto L300;
    fnorm1 = enorm(m,wa4);

#if BUG
    printf( "pnorm %.10e  fnorm1 %.10e\n", pnorm, fnorm1 );
#endif

    // compute the scaled actual reduction.

    actred = -one;
    if( (p1*fnorm1) < fnorm)
    {
        temp = fnorm1/fnorm;
        actred = one - temp * temp;
    }

    // compute the scaled predicted reduction and the scaled directional derivative.

    jj = 0;
    for( j=0; j<n; j++ )
    {
        wa3[j] = zero;
        l = ipvt[j];
        temp = wa1[l];
        ij = jj;
        for( i=0; i<=j; i++ )
        {
            wa3[i] += fjac[ij]*temp;
            ij += 1; // fjac[i+m*j]
        }
        jj += m;
    }
    temp1 = enorm(n,wa3)/fnorm;
    temp2 = (sqrt(par)*pnorm)/fnorm;
    prered = temp1*temp1 + (temp2*temp2)/p5;
    dirder = -(temp1*temp1 + temp2*temp2);

    // compute the ratio of the actual to the predicted reduction.

    ratio = zero;
    if(prered != zero)
        ratio = actred/prered;

    // update the step bound.

    if(ratio <= p25)
    {
        if(actred >= zero)
            temp = p5;
        else
            temp = p5*dirder/(dirder + p5*actred);

        if( ((p1*fnorm1) >= fnorm) || (temp < p1) )
            temp = p1;

        delta = temp*dmin1(delta,pnorm/p1);
        par = par/temp;
    }
    else
    {
        if( (par == zero) || (ratio >= p75) )
        {
            delta = pnorm/p5;
            par = p5*par;
        }
    }

    // test for successful iteration.

    if(ratio >= p0001)
    {
        // successful iteration. update x, fvec, and their norms.

        for( j=0; j<n; j++ )
        {
            x[j] = wa2[j];
            wa2[j] = diag[j]*x[j];
        }
        for( i=0; i<m; i++ )
            fvec[i] = wa4[i];
        xnorm = enorm(n,wa2);
        fnorm = fnorm1;
        iter += 1;
    }

    // tests for convergence.

    if( (fabs(actred) <= ftol) && (prered <= ftol) && (p5*ratio <= one) )
        *info = 1;
    if(delta <= xtol*xnorm)
        *info = 2;
    if( (fabs(actred) <= ftol) && (prered <= ftol) && (p5*ratio <= one) && ( *info == 2) )
        *info = 3;
    if( *info != 0)
        goto L300;

    // tests for termination and stringent tolerances.

    if( *nfev >= maxfev)
        *info = 5;
    if( (fabs(actred) <= m_eps) && (prered <= m_eps) && (p5*ratio <= one) )
        *info = 6;
    if(delta <= m_eps*xnorm)
        *info = 7;
    if(gnorm <= m_eps)
        *info = 8;
    if( *info != 0)
        goto L300;

    // end of the inner loop. repeat if iteration unsuccessful.

    if(ratio < p0001)
        goto L200;

    // end of the outer loop.

    goto L30;

L300:

    // termination, either normal or user imposed.

    if(iflag < 0)
        *info = iflag;
    iflag = 0;
    if(nprint > 0)
        fcn(m,n,x,fvec,&iflag);
}
/************************lmpar.c*************************/

#define BUG 0

/*     **********
*
*     subroutine lmpar
*
*     given an m by n matrix a, an n by n nonsingular diagonal
*     matrix d, an m-vector b, and a positive number delta,
*     the problem is to determine a value for the parameter
*     par such that if x solves the system
*
*       a*x = b ,     sqrt(par)*d*x = 0 ,
*
*     in the least squares sense, and dxnorm is the euclidean
*     norm of d*x, then either par is zero and
*
*       (dxnorm-delta) .le. 0.1*delta ,
*
*     or par is positive and
*
*       abs(dxnorm-delta) .le. 0.1*delta .
*
*     this subroutine completes the solution of the problem
*     if it is provided with the necessary information from the
*     qr factorization, with column pivoting, of a. that is, if
*     a*p = q*r, where p is a permutation matrix, q has orthogonal
*     columns, and r is an upper triangular matrix with diagonal
*     elements of nonincreasing magnitude, then lmpar expects
*     the full upper triangle of r, the permutation matrix p,
*     and the first n components of (q transpose)*b. on output
*     lmpar also provides an upper triangular matrix s such that
*
*        t   t           t
*       p *(a *a + par*d*d)*p = s *s .
*
*     s is employed within lmpar and may be of separate interest.
*
*     only a few iterations are generally needed for convergence
*     of the algorithm. if, however, the limit of 10 iterations
*     is reached, then the output par will contain the best
*     value obtained so far.
*
*     the subroutine statement is
*
*   subroutine lmpar(n,r,ldr,ipvt,diag,qtb,delta,par,x,sdiag,
*            wa1,wa2)
*
*     where
*
*   n is a positive integer input variable set to the order of r.
*
*   r is an n by n array. on input the full upper triangle
*     must contain the full upper triangle of the matrix r.
*     on output the full upper triangle is unaltered, and the
*     strict lower triangle contains the strict upper triangle
*     (transposed) of the upper triangular matrix s.
*
*   ldr is a positive integer input variable not less than n
*     which specifies the leading dimension of the array r.
*
*   ipvt is an integer input array of length n which defines the
*     permutation matrix p such that a*p = q*r. column j of p
*     is column ipvt(j) of the identity matrix.
*
*   diag is an input array of length n which must contain the
*     diagonal elements of the matrix d.
*
*   qtb is an input array of length n which must contain the first
*     n elements of the vector (q transpose)*b.
*
*   delta is a positive input variable which specifies an upper
*     bound on the euclidean norm of d*x.
*
*   par is a nonnegative variable. on input par contains an
*     initial estimate of the levenberg-marquardt parameter.
*     on output par contains the final estimate.
*
*   x is an output array of length n which contains the least
*     squares solution of the system a*x = b, sqrt(par)*d*x = 0,
*     for the output par.
*
*   sdiag is an output array of length n which contains the
*     diagonal elements of the upper triangular matrix s.
*
*   wa1 and wa2 are work arrays of length n.
*
*     subprograms called
*
*   minpack-supplied ... dpmpar,enorm,qrsolv
*
*   fortran-supplied ... dabs,dmax1,dmin1,dsqrt
*
*     argonne national laboratory. minpack project. march 1980.
*     burton s. garbow, kenneth e. hillstrom, jorge j. more
*
*     **********
*/

void LM_LeastSquare::lmpar(int n, double r[], int ldr, int ipvt[],
                           double diag[], double qtb[], double delta, double *par,
                           double x[], double sdiag[], double wa1[], double wa2[])
{
    int i,iter,ij,jj,j,jm1,jp1,k,l,nsing;
    double dxnorm,fp,gnorm,parc,parl,paru;
    double sum,temp;
    static double zero = 0.0;
    // static double one = 1.0;
    static double p1 = 0.1;
    static double p001 = 0.001;

#if BUG
    printf( "lmpar\n" );
#endif

    // compute and store in x the gauss-newton direction. if the
    // jacobian is rank-deficient, obtain a least squares solution.

    nsing = n;
    jj = 0;
    for( j=0; j<n; j++ )
    {
        wa1[j] = qtb[j];
        if( (r[jj] == zero) && (nsing == n) )
            nsing = j;
        if(nsing < n)
            wa1[j] = zero;
        jj += ldr+1; // [j+ldr*j]
    }
#if BUG
    printf( "nsing %d ", nsing );
#endif
    if(nsing >= 1)
    {
        for( k=0; k<nsing; k++ )
        {
            j = nsing - k - 1;
            wa1[j] = wa1[j]/r[j+ldr*j];
            temp = wa1[j];
            jm1 = j - 1;
            if(jm1 >= 0)
            {
                ij = ldr * j;
                for( i=0; i<=jm1; i++ )
                {
                    wa1[i] -= r[ij]*temp;
                    ij += 1;
                }
            }
        }
    }

    for( j=0; j<n; j++ )
    {
        l = ipvt[j];
        x[l] = wa1[j];
    }

    // initialize the iteration counter.
    // evaluate the function at the origin, and test for acceptance of the gauss-newton direction.

    iter = 0;
    for( j=0; j<n; j++ )
        wa2[j] = diag[j]*x[j];
    dxnorm = enorm(n,wa2);
    fp = dxnorm - delta;
    if(fp <= p1*delta)
    {
#if BUG
        printf( "going to L220\n" );
#endif
        goto L220;
    }

    // if the jacobian is not rank deficient, the newton
    // step provides a lower bound, parl, for the zero of
    // the function. otherwise set this bound to zero.

    parl = zero;
    if(nsing >= n)
    {
        for( j=0; j<n; j++ )
        {
            l = ipvt[j];
            wa1[j] = diag[l]*(wa2[l]/dxnorm);
        }
        jj = 0;
        for( j=0; j<n; j++ )
        {
            sum = zero;
            jm1 = j - 1;
            if(jm1 >= 0)
            {
                ij = jj;
                for( i=0; i<=jm1; i++ )
                {
                    sum += r[ij]*wa1[i];
                    ij += 1;
                }
            }
            wa1[j] = (wa1[j] - sum)/r[j+ldr*j];
            jj += ldr; // [i+ldr*j]
        }
        temp = enorm(n,wa1);
        parl = ((fp/delta)/temp)/temp;
    }

    // calculate an upper bound, paru, for the zero of the function.

    jj = 0;
    for( j=0; j<n; j++ )
    {
        sum = zero;
        ij = jj;
        for( i=0; i<=j; i++ )
        {
            sum += r[ij]*qtb[i];
            ij += 1;
        }
        l = ipvt[j];
        wa1[j] = sum/diag[l];
        jj += ldr; // [i+ldr*j]
    }
    gnorm = enorm(n,wa1);
    paru = gnorm/delta;
    if(paru == zero)
        paru = m_dwarf/dmin1(delta,p1);

    // if the input par lies outside of the interval (parl,paru),
    // set par to the closer endpoint.

    *par = dmax1( *par,parl);
    *par = dmin1( *par,paru);
    if( *par == zero)
        *par = gnorm/dxnorm;
#if BUG
    printf( "parl %.4e  par %.4e  paru %.4e\n", parl, *par, paru );
#endif

    // beginning of an iteration.

L150:
    iter += 1;

    // evaluate the function at the current value of par.

    if( *par == zero)
        *par = dmax1(m_dwarf,p001*paru);
    temp = sqrt( *par );
    for( j=0; j<n; j++ )
        wa1[j] = temp*diag[j];
    qrsolv(n,r,ldr,ipvt,wa1,qtb,x,sdiag,wa2);
    for( j=0; j<n; j++ )
        wa2[j] = diag[j]*x[j];
    dxnorm = enorm(n,wa2);
    temp = fp;
    fp = dxnorm - delta;

    // if the function is small enough, accept the current value
    // of par. also test for the exceptional cases where parl
    // is zero or the number of iterations has reached 10.

    if( (fabs(fp) <= p1*delta) || ((parl == zero) && (fp <= temp) && (temp < zero)) || (iter == 10) )
        goto L220;

    // compute the newton correction.

    for( j=0; j<n; j++ )
    {
        l = ipvt[j];
        wa1[j] = diag[l]*(wa2[l]/dxnorm);
    }
    jj = 0;
    for( j=0; j<n; j++ )
    {
        wa1[j] = wa1[j]/sdiag[j];
        temp = wa1[j];
        jp1 = j + 1;
        if(jp1 < n)
        {
            ij = jp1 + jj;
            for( i=jp1; i<n; i++ )
            {
                wa1[i] -= r[ij]*temp;
                ij += 1; // [i+ldr*j]
            }
        }
        jj += ldr; // ldr*j
    }
    temp = enorm(n,wa1);
    parc = ((fp/delta)/temp)/temp;

    // depending on the sign of the function, update parl or paru.

    if(fp > zero)
        parl = dmax1(parl, *par);
    if(fp < zero)
        paru = dmin1(paru, *par);

    // compute an improved estimate for par.

    *par = dmax1(parl, *par + parc);

    // end of an iteration.

    goto L150;

L220:

    // termination.

    if(iter == 0)
        *par = zero;
}
/************************qrfac.c*************************/

#define BUG 0

/*
*     **********
*
*     subroutine qrfac
*
*     this subroutine uses householder transformations with column
*     pivoting (optional) to compute a qr factorization of the
*     m by n matrix a. that is, qrfac determines an orthogonal
*     matrix q, a permutation matrix p, and an upper trapezoidal
*     matrix r with diagonal elements of nonincreasing magnitude,
*     such that a*p = q*r. the householder transformation for
*     column k, k = 1,2,...,min(m,n), is of the form
*
*               t
*       i - (1/u(k))*u*u
*
*     where u has zeros in the first k-1 positions. the form of
*     this transformation and the method of pivoting first
*     appeared in the corresponding linpack subroutine.
*
*     the subroutine statement is
*
*   subroutine qrfac(m,n,a,lda,pivot,ipvt,lipvt,rdiag,acnorm,wa)
*
*     where
*
*   m is a positive integer input variable set to the number
*     of rows of a.
*
*   n is a positive integer input variable set to the number
*     of columns of a.
*
*   a is an m by n array. on input a contains the matrix for
*     which the qr factorization is to be computed. on output
*     the strict upper trapezoidal part of a contains the strict
*     upper trapezoidal part of r, and the lower trapezoidal
*     part of a contains a factored form of q (the non-trivial
*     elements of the u vectors described above).
*
*   lda is a positive integer input variable not less than m
*     which specifies the leading dimension of the array a.
*
*   pivot is a logical input variable. if pivot is set true,
*     then column pivoting is enforced. if pivot is set false,
*     then no column pivoting is done.
*
*   ipvt is an integer output array of length lipvt. ipvt
*     defines the permutation matrix p such that a*p = q*r.
*     column j of p is column ipvt(j) of the identity matrix.
*     if pivot is false, ipvt is not referenced.
*
*   lipvt is a positive integer input variable. if pivot is false,
*     then lipvt may be as small as 1. if pivot is true, then
*     lipvt must be at least n.
*
*   rdiag is an output array of length n which contains the
*     diagonal elements of r.
*
*   acnorm is an output array of length n which contains the
*     norms of the corresponding columns of the input matrix a.
*     if this information is not needed, then acnorm can coincide
*     with rdiag.
*
*   wa is a work array of length n. if pivot is false, then wa
*     can coincide with rdiag.
*
*     subprograms called
*
*   minpack-supplied ... dpmpar,enorm
*
*   fortran-supplied ... dmax1,dsqrt,min0
*
*     argonne national laboratory. minpack project. march 1980.
*     burton s. garbow, kenneth e. hillstrom, jorge j. more
*
*     **********
*/

void LM_LeastSquare::qrfac(int m, int n, double a[], int WXUNUSED(lda), int pivot, int ipvt[],
                           int WXUNUSED(lipvt), double rdiag[], double acnorm[], double wa[])
{
    int i,ij,jj,j,jp1,k,kmax,minmn;
    double ajnorm,sum,temp;
    static double zero = 0.0;
    static double one = 1.0;
    static double p05 = 0.05;

    // compute the initial column norms and initialize several arrays.

    ij = 0;
    for( j=0; j<n; j++ )
    {
        acnorm[j] = enorm(m,&a[ij]);
        rdiag[j] = acnorm[j];
        wa[j] = rdiag[j];
        if(pivot != 0)
            ipvt[j] = j;
        ij += m; // m*j
    }
#if BUG
    printf( "qrfac\n" );
#endif

    // reduce a to r with householder transformations.

    minmn = min0(m,n);
    for( j=0; j<minmn; j++ )
    {
        if(pivot == 0)
            goto L40;

    // bring the column of largest norm into the pivot position.

        kmax = j;
        for( k=j; k<n; k++ )
        {
        if(rdiag[k] > rdiag[kmax])
            kmax = k;
        }
        if(kmax == j)
            goto L40;

        ij = m * j;
        jj = m * kmax;
        for( i=0; i<m; i++ )
        {
            temp = a[ij];  // [i+m*j]
            a[ij] = a[jj]; // [i+m*kmax]
            a[jj] = temp;
            ij += 1;
            jj += 1;
        }
        rdiag[kmax] = rdiag[j];
        wa[kmax] = wa[j];
        k = ipvt[j];
        ipvt[j] = ipvt[kmax];
        ipvt[kmax] = k;

L40:

        // compute the householder transformation to reduce the
        // j-th column of a to a multiple of the j-th unit vector.

        jj = j + m*j;
        ajnorm = enorm(m-j,&a[jj]);
        if(ajnorm == zero)
            goto L100;
        if(a[jj] < zero)
            ajnorm = -ajnorm;
        ij = jj;
        for( i=j; i<m; i++ )
        {
            a[ij] /= ajnorm;
            ij += 1; // [i+m*j]
        }
        a[jj] += one;

        // apply the transformation to the remaining columns and update the norms.

        jp1 = j + 1;
        if(jp1 < n )
        {
            for( k=jp1; k<n; k++ )
            {
                sum = zero;
                ij = j + m*k;
                jj = j + m*j;
                for( i=j; i<m; i++ )
                {
                    sum += a[jj]*a[ij];
                    ij += 1; // [i+m*k]
                    jj += 1; // [i+m*j]
                }
                temp = sum/a[j+m*j];
                ij = j + m*k;
                jj = j + m*j;
                for( i=j; i<m; i++ )
                {
                    a[ij] -= temp*a[jj];
                    ij += 1; // [i+m*k]
                    jj += 1; // [i+m*j]
                }
                if( (pivot != 0) && (rdiag[k] != zero) )
                {
                    temp = a[j+m*k]/rdiag[k];
                    temp = dmax1( zero, one-temp*temp );
                    rdiag[k] *= sqrt(temp);
                    temp = rdiag[k]/wa[k];
                    if( (p05*temp*temp) <= m_eps)
                    {
                        rdiag[k] = enorm(m-j-1,&a[jp1+m*k]);
                        wa[k] = rdiag[k];
                    }
                }
            }
        }

L100:
        rdiag[j] = -ajnorm;
    }
}
/************************qrsolv.c*************************/

#define BUG 0
/*
*     **********
*
*     subroutine qrsolv
*
*     given an m by n matrix a, an n by n diagonal matrix d,
*     and an m-vector b, the problem is to determine an x which
*     solves the system
*
*       a*x = b ,     d*x = 0 ,
*
*     in the least squares sense.
*
*     this subroutine completes the solution of the problem
*     if it is provided with the necessary information from the
*     qr factorization, with column pivoting, of a. that is, if
*     a*p = q*r, where p is a permutation matrix, q has orthogonal
*     columns, and r is an upper triangular matrix with diagonal
*     elements of nonincreasing magnitude, then qrsolv expects
*     the full upper triangle of r, the permutation matrix p,
*     and the first n components of (q transpose)*b. the system
*     a*x = b, d*x = 0, is then equivalent to
*
*          t       t
*       r*z = q *b ,  p *d*p*z = 0 ,
*
*     where x = p*z. if this system does not have full rank,
*     then a least squares solution is obtained. on output qrsolv
*     also provides an upper triangular matrix s such that
*
*        t   t       t
*       p *(a *a + d*d)*p = s *s .
*
*     s is computed within qrsolv and may be of separate interest.
*
*     the subroutine statement is
*
*   subroutine qrsolv(n,r,ldr,ipvt,diag,qtb,x,sdiag,wa)
*
*     where
*
*   n is a positive integer input variable set to the order of r.
*
*   r is an n by n array. on input the full upper triangle
*     must contain the full upper triangle of the matrix r.
*     on output the full upper triangle is unaltered, and the
*     strict lower triangle contains the strict upper triangle
*     (transposed) of the upper triangular matrix s.
*
*   ldr is a positive integer input variable not less than n
*     which specifies the leading dimension of the array r.
*
*   ipvt is an integer input array of length n which defines the
*     permutation matrix p such that a*p = q*r. column j of p
*     is column ipvt(j) of the identity matrix.
*
*   diag is an input array of length n which must contain the
*     diagonal elements of the matrix d.
*
*   qtb is an input array of length n which must contain the first
*     n elements of the vector (q transpose)*b.
*
*   x is an output array of length n which contains the least
*     squares solution of the system a*x = b, d*x = 0.
*
*   sdiag is an output array of length n which contains the
*     diagonal elements of the upper triangular matrix s.
*
*   wa is a work array of length n.
*
*     subprograms called
*
*   fortran-supplied ... dabs,dsqrt
*
*     argonne national laboratory. minpack project. march 1980.
*     burton s. garbow, kenneth e. hillstrom, jorge j. more
*
*     **********
*/

void LM_LeastSquare::qrsolv(int n, double r[], int ldr, int ipvt[], double diag[],
                            double qtb[], double x[], double sdiag[], double wa[])
{
    int i, ij, ik, kk, j, jp1, k, kp1, l, nsing;
    double cos, cotan, qtbpj, sin, sum, tan, temp;
    static double zero = 0.0;
    static double p25 = 0.25;
    static double p5 = 0.5;

    // copy r and (q transpose)*b to preserve input and initialize s.
    // in particular, save the diagonal elements of r in x.

    kk = 0;
    for( j=0; j<n; j++ )
    {
        ij = kk;
        ik = kk;
        for( i=j; i<n; i++ )
        {
            r[ij] = r[ik];
            ij += 1;   // [i+ldr*j]
            ik += ldr; // [j+ldr*i]
        }
        x[j] = r[kk];
        wa[j] = qtb[j];
        kk += ldr+1; // j+ldr*j
    }
#if BUG
    printf( "qrsolv\n" );
#endif

    // eliminate the diagonal matrix d using a givens rotation.

    for( j=0; j<n; j++ )
    {

        // prepare the row of d to be eliminated, locating the
        // diagonal element using p from the qr factorization.

        l = ipvt[j];
        if(diag[l] == zero)
            goto L90;
        for( k=j; k<n; k++ )
            sdiag[k] = zero;
        sdiag[j] = diag[l];

        // the transformations to eliminate the row of d modify only a single
        // element of (q transpose)*b beyond the first n, which is initially zero.

        qtbpj = zero;
        for( k=j; k<n; k++ )
        {
            // determine a givens rotation which eliminates the
            // appropriate element in the current row of d.

            if(sdiag[k] == zero)
                continue;
            kk = k + ldr * k;
            if(fabs(r[kk]) < fabs(sdiag[k]))
            {
                cotan = r[kk]/sdiag[k];
                sin = p5/sqrt(p25+p25*cotan*cotan);
                cos = sin*cotan;
            }
            else
            {
                tan = sdiag[k]/r[kk];
                cos = p5/sqrt(p25+p25*tan*tan);
                sin = cos*tan;
            }

            // compute the modified diagonal element of r and
            // the modified element of ((q transpose)*b,0).

            r[kk] = cos*r[kk] + sin*sdiag[k];
            temp = cos*wa[k] + sin*qtbpj;
            qtbpj = -sin*wa[k] + cos*qtbpj;
            wa[k] = temp;

            // accumulate the tranformation in the row of s.

            kp1 = k + 1;
            if( n > kp1 )
            {
                ik = kk + 1;
                for( i=kp1; i<n; i++ )
                {
                    temp = cos*r[ik] + sin*sdiag[i];
                    sdiag[i] = -sin*r[ik] + cos*sdiag[i];
                    r[ik] = temp;
                    ik += 1; // [i+ldr*k]
                }
            }
        }
L90:

        // store the diagonal element of s and restore
        // the corresponding diagonal element of r.

        kk = j + ldr*j;
        sdiag[j] = r[kk];
        r[kk] = x[j];
    }

    // solve the triangular system for z. if the system is
    // singular, then obtain a least squares solution.

    nsing = n;
    for( j=0; j<n; j++ )
    {
        if( (sdiag[j] == zero) && (nsing == n) )
            nsing = j;
        if(nsing < n)
            wa[j] = zero;
    }
    if(nsing < 1)
        goto L150;

    for( k=0; k<nsing; k++ )
    {
        j = nsing - k - 1;
        sum = zero;
        jp1 = j + 1;
        if(nsing > jp1)
        {
            ij = jp1 + ldr * j;
            for( i=jp1; i<nsing; i++ )
            {
                sum += r[ij]*wa[i];
                ij += 1; // [i+ldr*j]
            }
        }
        wa[j] = (wa[j] - sum)/sdiag[j];
    }
L150:

    // permute the components of z back to components of x.

    for( j=0; j<n; j++ )
    {
        l = ipvt[j];
        x[l] = wa[j];
    }
}
/************************enorm.c*************************/

/*
*     **********
*
*     function enorm
*
*     given an n-vector x, this function calculates the
*     euclidean norm of x.
*
*     the euclidean norm is computed by accumulating the sum of
*     squares in three different sums. the sums of squares for the
*     small and large components are scaled so that no overflows
*     occur. non-destructive underflows are permitted. underflows
*     and overflows do not occur in the computation of the unscaled
*     sum of squares for the intermediate components.
*     the definitions of small, intermediate and large components
*     depend on two constants, rdwarf and rgiant. the main
*     restrictions on these constants are that rdwarf**2 not
*     underflow and rgiant**2 not overflow. the constants
*     given here are suitable for every known computer.
*
*     the function statement is
*
*   double precision function enorm(n,x)
*
*     where
*
*   n is a positive integer input variable.
*
*   x is an input array of length n.
*
*     subprograms called
*
*   fortran-supplied ... dabs,dsqrt
*
*     argonne national laboratory. minpack project. march 1980.
*     burton s. garbow, kenneth e. hillstrom, jorge j. more
*
*     **********
*/

double LM_LeastSquare::enorm(int n, double x[])
{
    int i;
    double agiant, floatn, s1, s2, s3, xabs, x1max, x3max;
    double ans, temp;
    static double rdwarf = 3.834e-20;
    static double rgiant = 1.304e19;
    static double zero   = 0.0;
    static double one    = 1.0;

    s1 = zero;
    s2 = zero;
    s3 = zero;
    x1max = zero;
    x3max = zero;
    floatn = n;
    agiant = rgiant/floatn;

    for( i=0; i<n; i++ )
    {
        xabs = fabs(x[i]);
        if( (xabs > rdwarf) && (xabs < agiant) )
        {
            // sum for intermediate components.
            s2 += xabs*xabs;
            continue;
        }

        if(xabs > rdwarf)
        {
            // sum for large components.
            if(xabs > x1max)
            {
                temp = x1max/xabs;
                s1 = one + s1*temp*temp;
                x1max = xabs;
            }
            else
            {
                temp = xabs/x1max;
                s1 += temp*temp;
            }
            continue;
        }

        // sum for small components.
        if(xabs > x3max)
        {
            temp = x3max/xabs;
            s3 = one + s3*temp*temp;
            x3max = xabs;
        }
        else
        {
            if(xabs != zero)
            {
                temp = xabs/x3max;
                s3 += temp*temp;
            }
        }
    }

    // calculation of norm.
    if(s1 != zero)
    {
        temp = s1 + (s2/x1max)/x1max;
        ans = x1max*sqrt(temp);
        return(ans);
    }
    if(s2 != zero)
    {
        if(s2 >= x3max)
            temp = s2*(one+(x3max/s2)*(x3max*s3));
        else
            temp = x3max*((s2/x3max)+(x3max*s3));
        ans = sqrt(temp);
    }
    else
    {
        ans = x3max*sqrt(s3);
    }
    return(ans);
}

/************************fdjac2.c*************************/

#define BUG 0

/*
*     **********
*
*     subroutine fdjac2
*
*     this subroutine computes a forward-difference approximation
*     to the m by n jacobian matrix associated with a specified
*     problem of m functions in n variables.
*
*     the subroutine statement is
*
*   subroutine fdjac2(fcn,m,n,x,fvec,fjac,ldfjac,iflag,epsfcn,wa)
*
*     where
*
*   fcn is the name of the user-supplied subroutine which
*     calculates the functions. fcn must be declared
*     in an external statement in the user calling
*     program, and should be written as follows.
*
*     subroutine fcn(m,n,x,fvec,iflag)
*     integer m,n,iflag
*     double precision x(n),fvec(m)
*     ----------
*     calculate the functions at x and
*     return this vector in fvec.
*     ----------
*     return
*     end
*
*     the value of iflag should not be changed by fcn unless
*     the user wants to terminate execution of fdjac2.
*     in this case set iflag to a negative integer.
*
*   m is a positive integer input variable set to the number
*     of functions.
*
*   n is a positive integer input variable set to the number
*     of variables. n must not exceed m.
*
*   x is an input array of length n.
*
*   fvec is an input array of length m which must contain the
*     functions evaluated at x.
*
*   fjac is an output m by n array which contains the
*     approximation to the jacobian matrix evaluated at x.
*
*   ldfjac is a positive integer input variable not less than m
*     which specifies the leading dimension of the array fjac.
*
*   iflag is an integer variable which can be used to terminate
*     the execution of fdjac2. see description of fcn.
*
*   epsfcn is an input variable used in determining a suitable
*     step length for the forward-difference approximation. this
*     approximation assumes that the relative errors in the
*     functions are of the order of epsfcn. if epsfcn is less
*     than the machine precision, it is assumed that the relative
*     errors in the functions are of the order of the machine
*     precision.
*
*   wa is a work array of length m.
*
*     subprograms called
*
*   user-supplied ...... fcn
*
*   minpack-supplied ... dpmpar
*
*   fortran-supplied ... dabs,dmax1,dsqrt
*
*     argonne national laboratory. minpack project. march 1980.
*     burton s. garbow, kenneth e. hillstrom, jorge j. more
*
      **********
*/

void LM_LeastSquare::fdjac2(int m,int n, double x[], double fvec[], double fjac[],
                            int WXUNUSED(ldfjac), int *iflag, double epsfcn, double wa[])
{
    int i,j,ij;
    double eps,h,temp;
    static double zero = 0.0;

    temp = dmax1(epsfcn,m_eps);
    eps = sqrt(temp);
#if BUG
    printf( "fdjac2\n" );
#endif
    ij = 0;
    for( j=0; j<n; j++ )
    {
        temp = x[j];
        h = eps * fabs(temp);
        if(h == zero)
            h = eps;
        x[j] = temp + h;
        fcn(m,n,x,wa,iflag);
        if( *iflag < 0)
            return;
        x[j] = temp;
        for( i=0; i<m; i++ )
        {
            fjac[ij] = (wa[i] - fvec[i])/h;
            ij += 1;    // fjac[i+m*j]
        }
    }
#if BUG
    pmat( m, n, fjac );
#endif
}
