#include <complex>

#include "fluxml_config.h"
#include "Error.h"
#include "LAPackWrap.h"

// Fortran-Prototypen
#define DGESVD_F77	F77_FUNC(dgesvd,DGESVD)
#define DGEEVX_F77	F77_FUNC(dgeevx,DGEEVX)
#define DGETRF_F77	F77_FUNC(dgetrf,DGETRF)
#define DGETRS_F77	F77_FUNC(dgetrs,DGETRS)
#define DGETRI_F77	F77_FUNC(dgetri,DGETRI)
#define DGESV_F77	F77_FUNC(dgesv,DGESV)
#define DGESVX_F77	F77_FUNC(dgesvx,DGESVX)
#define DGEQRF_F77	F77_FUNC(dgeqrf,DGEQRF)
#define DGEQP3_F77	F77_FUNC(dgeqp3,DGEQP3)
#define DORGQR_F77	F77_FUNC(dorgqr,DORGQR)
#define DORMQR_F77	F77_FUNC(dormqr,DORMQR)
#if 0
#define DGELSY_F77	F77_FUNC(dgelsy,DGELSY)
#endif
#define DGELS_F77	F77_FUNC(dgels,DGELS)
#define DGELSS_F77	F77_FUNC(dgelss,DGELSS)
#define ZGEES_F77	F77_FUNC(zgees,ZGEES)
#define DPOTRF_F77	F77_FUNC(dpotrf,DPOTRF)
#define DSYTRD_F77	F77_FUNC(dsytrd,DSYTRD)
#define DSTEBZ_F77	F77_FUNC(dstebz,DSTEBZ)

extern "C" {
// Purpose
// =======
//
// DGESVD computes the singular value decomposition (SVD) of a real
// M-by-N matrix A, optionally computing the left and/or right singular
// vectors. The SVD is written
//
//      A = U * SIGMA * transpose(V)
//
// where SIGMA is an M-by-N matrix which is zero except for its
// min(m,n) diagonal elements, U is an M-by-M orthogonal matrix, and
// V is an N-by-N orthogonal matrix.  The diagonal elements of SIGMA
// are the singular values of A; they are real and non-negative, and
// are returned in descending order.  The first min(m,n) columns of
// U and V are the left and right singular vectors of A.
//
// Note that the routine returns V**T, not V.
//
// Arguments
// =========
//
// JOBU    (input) CHARACTER*1
//         Specifies options for computing all or part of the matrix U:
//         = 'A':  all M columns of U are returned in array U:
//         = 'S':  the first min(m,n) columns of U (the left singular
//                 vectors) are returned in the array U;
//         = 'O':  the first min(m,n) columns of U (the left singular
//                 vectors) are overwritten on the array A;
//         = 'N':  no columns of U (no left singular vectors) are
//                 computed.
//
// JOBVT   (input) CHARACTER*1
//         Specifies options for computing all or part of the matrix
//         V**T:
//         = 'A':  all N rows of V**T are returned in the array VT;
//         = 'S':  the first min(m,n) rows of V**T (the right singular
//                 vectors) are returned in the array VT;
//         = 'O':  the first min(m,n) rows of V**T (the right singular
//                 vectors) are overwritten on the array A;
//         = 'N':  no rows of V**T (no right singular vectors) are
//                 computed.
//
//         JOBVT and JOBU cannot both be 'O'.
//
// M       (input) INTEGER
//         The number of rows of the input matrix A.  M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the input matrix A.  N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the M-by-N matrix A.
//         On exit,
//         if JOBU = 'O',  A is overwritten with the first min(m,n)
//                         columns of U (the left singular vectors,
//                         stored columnwise);
//         if JOBVT = 'O', A is overwritten with the first min(m,n)
//                         rows of V**T (the right singular vectors,
//                         stored rowwise);
//         if JOBU .ne. 'O' and JOBVT .ne. 'O', the contents of A
//                         are destroyed.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,M).
//
// S       (output) DOUBLE PRECISION array, dimension (min(M,N))
//         The singular values of A, sorted so that S(i) >= S(i+1).
//
// U       (output) DOUBLE PRECISION array, dimension (LDU,UCOL)
//         (LDU,M) if JOBU = 'A' or (LDU,min(M,N)) if JOBU = 'S'.
//         If JOBU = 'A', U contains the M-by-M orthogonal matrix U;
//         if JOBU = 'S', U contains the first min(m,n) columns of U
//         (the left singular vectors, stored columnwise);
//         if JOBU = 'N' or 'O', U is not referenced.
//
// LDU     (input) INTEGER
//         The leading dimension of the array U.  LDU >= 1; if
//         JOBU = 'S' or 'A', LDU >= M.
//
// VT      (output) DOUBLE PRECISION array, dimension (LDVT,N)
//         If JOBVT = 'A', VT contains the N-by-N orthogonal matrix
//         V**T;
//         if JOBVT = 'S', VT contains the first min(m,n) rows of
//         V**T (the right singular vectors, stored rowwise);
//         if JOBVT = 'N' or 'O', VT is not referenced.
//
// LDVT    (input) INTEGER
//         The leading dimension of the array VT.  LDVT >= 1; if
//         JOBVT = 'A', LDVT >= N; if JOBVT = 'S', LDVT >= min(M,N).
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK;
//         if INFO > 0, WORK(2:MIN(M,N)) contains the unconverged
//         superdiagonal elements of an upper bidiagonal matrix B
//         whose diagonal is in S (not necessarily sorted). B
//         satisfies A = U * B * VT, so it has the same singular values
//         as A, and singular vectors related by U and VT.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.
//         LWORK >= MAX(1,3*MIN(M,N)+MAX(M,N),5*MIN(M,N)).
//         For good performance, LWORK should generally be larger.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0:  successful exit.
//         < 0:  if INFO = -i, the i-th argument had an illegal value.
//         > 0:  if DBDSQR did not converge, INFO specifies how many
//               superdiagonals of an intermediate bidiagonal form B
//               did not converge to zero. See the description of WORK
//               above for details.
void DGESVD_F77(
	char * JOBU,		// CHARACTER
	char * JOBVT,		// CHARACTER
	int * M,		// INTEGER
	int * N,		// INTEGER
	double * A,		// DOUBLE PRECISION A( LDA, * )
	int * LDA,		// INTEGER
	double * S,		// DOUBLE PRECISION S( * )
	double * U,		// DOUBLE PRECISION U( LDU, * )
	int * LDU,		// INTEGER
	double * VT,		// DOUBLE PRECISION VT( LDVT, * )
	int * LDVT,		// INTEGER
	double * WORK,		// DOUBLE PRECISION WORK ( * )
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGEEVX computes for an N-by-N real nonsymmetric matrix A, the
// eigenvalues and, optionally, the left and/or right eigenvectors.
//
// Optionally also, it computes a balancing transformation to improve
// the conditioning of the eigenvalues and eigenvectors (ILO, IHI,
// SCALE, and ABNRM), reciprocal condition numbers for the eigenvalues
// (RCONDE), and reciprocal condition numbers for the right
// eigenvectors (RCONDV).
//
// The right eigenvector v(j) of A satisfies
//                  A * v(j) = lambda(j) * v(j)
// where lambda(j) is its eigenvalue.
// The left eigenvector u(j) of A satisfies
//               u(j)**H * A = lambda(j) * u(j)**H
// where u(j)**H denotes the conjugate transpose of u(j).
//
// The computed eigenvectors are normalized to have Euclidean norm
// equal to 1 and largest component real.
//
// Balancing a matrix means permuting the rows and columns to make it
// more nearly upper triangular, and applying a diagonal similarity
// transformation D * A * D**(-1), where D is a diagonal matrix, to
// make its rows and columns closer in norm and the condition numbers
// of its eigenvalues and eigenvectors smaller.  The computed
// reciprocal condition numbers correspond to the balanced matrix.
// Permuting rows and columns will not change the condition numbers
// (in exact arithmetic) but diagonal scaling will.  For further
// explanation of balancing, see section 4.10.2 of the LAPACK
// Users' Guide.
//
// Arguments
// =========
//
// BALANC  (input) CHARACTER*1
//         Indicates how the input matrix should be diagonally scaled
//         and/or permuted to improve the conditioning of its
//         eigenvalues.
//         = 'N': Do not diagonally scale or permute;
//         = 'P': Perform permutations to make the matrix more nearly
//                upper triangular. Do not diagonally scale;
//         = 'S': Diagonally scale the matrix, i.e. replace A by
//                D*A*D**(-1), where D is a diagonal matrix chosen
//                to make the rows and columns of A more equal in
//                norm. Do not permute;
//         = 'B': Both diagonally scale and permute A.
//
//         Computed reciprocal condition numbers will be for the matrix
//         after balancing and/or permuting. Permuting does not change
//         condition numbers (in exact arithmetic), but balancing does.
//
// JOBVL   (input) CHARACTER*1
//         = 'N': left eigenvectors of A are not computed;
//         = 'V': left eigenvectors of A are computed.
//         If SENSE = 'E' or 'B', JOBVL must = 'V'.
//
// JOBVR   (input) CHARACTER*1
//         = 'N': right eigenvectors of A are not computed;
//         = 'V': right eigenvectors of A are computed.
//         If SENSE = 'E' or 'B', JOBVR must = 'V'.
//
// SENSE   (input) CHARACTER*1
//         Determines which reciprocal condition numbers are computed.
//         = 'N': None are computed;
//         = 'E': Computed for eigenvalues only;
//         = 'V': Computed for right eigenvectors only;
//         = 'B': Computed for eigenvalues and right eigenvectors.
//
//         If SENSE = 'E' or 'B', both left and right eigenvectors
//         must also be computed (JOBVL = 'V' and JOBVR = 'V').
//
// N       (input) INTEGER
//         The order of the matrix A. N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the N-by-N matrix A.
//         On exit, A has been overwritten.  If JOBVL = 'V' or
//         JOBVR = 'V', A contains the real Schur form of the balanced
//         version of the input matrix A.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// WR      (output) DOUBLE PRECISION array, dimension (N)
// WI      (output) DOUBLE PRECISION array, dimension (N)
//         WR and WI contain the real and imaginary parts,
//         respectively, of the computed eigenvalues.  Complex
//         conjugate pairs of eigenvalues will appear consecutively
//         with the eigenvalue having the positive imaginary part
//         first.
//
// VL      (output) DOUBLE PRECISION array, dimension (LDVL,N)
//         If JOBVL = 'V', the left eigenvectors u(j) are stored one
//         after another in the columns of VL, in the same order
//         as their eigenvalues.
//         If JOBVL = 'N', VL is not referenced.
//         If the j-th eigenvalue is real, then u(j) = VL(:,j),
//         the j-th column of VL.
//         If the j-th and (j+1)-st eigenvalues form a complex
//         conjugate pair, then u(j) = VL(:,j) + i*VL(:,j+1) and
//         u(j+1) = VL(:,j) - i*VL(:,j+1).
//
// LDVL    (input) INTEGER
//         The leading dimension of the array VL.  LDVL >= 1; if
//         JOBVL = 'V', LDVL >= N.
//
// VR      (output) DOUBLE PRECISION array, dimension (LDVR,N)
//         If JOBVR = 'V', the right eigenvectors v(j) are stored one
//         after another in the columns of VR, in the same order
//         as their eigenvalues.
//         If JOBVR = 'N', VR is not referenced.
//         If the j-th eigenvalue is real, then v(j) = VR(:,j),
//         the j-th column of VR.
//         If the j-th and (j+1)-st eigenvalues form a complex
//         conjugate pair, then v(j) = VR(:,j) + i*VR(:,j+1) and
//         v(j+1) = VR(:,j) - i*VR(:,j+1).
//
// LDVR    (input) INTEGER
//         The leading dimension of the array VR.  LDVR >= 1, and if
//         JOBVR = 'V', LDVR >= N.
//
// ILO     (output) INTEGER
// IHI     (output) INTEGER
//         ILO and IHI are integer values determined when A was
//         balanced.  The balanced A(i,j) = 0 if I > J and
//         J = 1,...,ILO-1 or I = IHI+1,...,N.
//
// SCALE   (output) DOUBLE PRECISION array, dimension (N)
//         Details of the permutations and scaling factors applied
//         when balancing A.  If P(j) is the index of the row and column
//         interchanged with row and column j, and D(j) is the scaling
//         factor applied to row and column j, then
//         SCALE(J) = P(J),    for J = 1,...,ILO-1
//                  = D(J),    for J = ILO,...,IHI
//                  = P(J)     for J = IHI+1,...,N.
//         The order in which the interchanges are made is N to IHI+1,
//         then 1 to ILO-1.
//
// ABNRM   (output) DOUBLE PRECISION
//         The one-norm of the balanced matrix (the maximum
//         of the sum of absolute values of elements of any column).
//
// RCONDE  (output) DOUBLE PRECISION array, dimension (N)
//         RCONDE(j) is the reciprocal condition number of the j-th
//         eigenvalue.
//
// RCONDV  (output) DOUBLE PRECISION array, dimension (N)
//         RCONDV(j) is the reciprocal condition number of the j-th
//         right eigenvector.
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.   If SENSE = 'N' or 'E',
//         LWORK >= max(1,2*N), and if JOBVL = 'V' or JOBVR = 'V',
//         LWORK >= 3*N.  If SENSE = 'V' or 'B', LWORK >= N*(N+6).
//         For good performance, LWORK must generally be larger.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// IWORK   (workspace) INTEGER array, dimension (2*N-2)
//         If SENSE = 'N' or 'E', not referenced.
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value.
//         > 0:  if INFO = i, the QR algorithm failed to compute all the
//               eigenvalues, and no eigenvectors or condition numbers
//               have been computed; elements 1:ILO-1 and i+1:N of WR
//               and WI contain eigenvalues which have converged.
void DGEEVX_F77(
	char * BALANC,		// CHARACTER
	char * JOBVL,		// CHARACTER
	char * JOBVR,		// CHARACTER
	char * SENSE,		// CHARACTER
	int * N,		// INTEGER
	double * A,		// DOUBLE PRECISION A( LDA, * )
	int * LDA,		// INTEGER
	double * WR,		// DOUBLE PRECISION WR( * )
	double * WI,		// DOUBLE PRECISION WI( * )
	double * VL,		// DOUBLE PRECISION VL( LDVL, * )
	int * LDVL,		// INTEGER
	double * VR,		// DOUBLE PRECISION VR( LDVR, * )
	int * LDVR,		// INTEGER
	int * ILO,		// INTEGER
	int * IHI,		// INTEGER
	double * SCALE,		// DOUBLE PRECISION SCALE( * )
	double * ABNRM,		// DOUBLE PRECISION
	double * RCONDE,	// DOUBLE PRECISION RCONDE( * )
	double * RCONDV,	// DOUBLE PRECISION RCONDV( * )
	double * WORK,		// DOUBLE PRECISION WORK( * )
	int * LWORK,		// INTEGER
	int * IWORK,		// INTEGER IWORK( * )
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGETRF computes an LU factorization of a general M-by-N matrix A
// using partial pivoting with row interchanges.
//
// The factorization has the form
//    A = P * L * U
// where P is a permutation matrix, L is lower triangular with unit
// diagonal elements (lower trapezoidal if m > n), and U is upper
// triangular (upper trapezoidal if m < n).
//
// This is the right-looking Level 3 BLAS version of the algorithm.
//
// Arguments
// =========
//
// M       (input) INTEGER
//         The number of rows of the matrix A.  M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the matrix A.  N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the M-by-N matrix to be factored.
//         On exit, the factors L and U from the factorization
//         A = P*L*U; the unit diagonal elements of L are not stored.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,M).
//
// IPIV    (output) INTEGER array, dimension (min(M,N))
//         The pivot indices; for 1 <= i <= min(M,N), row i of the
//         matrix was interchanged with row IPIV(i).
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//         > 0:  if INFO = i, U(i,i) is exactly zero. The factorization
//               has been completed, but the factor U is exactly
//               singular, and division by zero will occur if it is used
//               to solve a system of equations.
void DGETRF_F77(
	int * M,		// INTEGER
	int * N,		// INTEGER
	double * A,		// DOUBLE PRECISION
	int * LDA,		// INTEGER
	unsigned int * IPIV,	// INTEGER IPIV( * )
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGETRS solves a system of linear equations
//    A * X = B  or  A' * X = B
// with a general N-by-N matrix A using the LU factorization computed
// by DGETRF.
//
// Arguments
// =========
//
// TRANS   (input) CHARACTER*1
//         Specifies the form of the system of equations:
//         = 'N':  A * X = B  (No transpose)
//         = 'T':  A'* X = B  (Transpose)
//         = 'C':  A'* X = B  (Conjugate transpose = Transpose)
//
// N       (input) INTEGER
//         The order of the matrix A.  N >= 0.
//
// NRHS    (input) INTEGER
//         The number of right hand sides, i.e., the number of columns
//         of the matrix B.  NRHS >= 0.
//
// A       (input) DOUBLE PRECISION array, dimension (LDA,N)
//         The factors L and U from the factorization A = P*L*U
//         as computed by DGETRF.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// IPIV    (input) INTEGER array, dimension (N)
//         The pivot indices from DGETRF; for 1<=i<=N, row i of the
//         matrix was interchanged with row IPIV(i).
//
// B       (input/output) DOUBLE PRECISION array, dimension (LDB,NRHS)
//         On entry, the right hand side matrix B.
//         On exit, the solution matrix X.
//
// LDB     (input) INTEGER
//         The leading dimension of the array B.  LDB >= max(1,N).
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
void DGETRS_F77(
	char * TRANS,		// CHARACTER
	int * N,		// INTEGER
	int * NRHS,		// INTEGER
	double * A,		// DOUBLE PRECISION
	int * LDA,		// INTEGER
	unsigned int * IPIV,	// INTEGER IPIV( * )
	double * B,		// DOUBLE PRECISION
	int * LDB,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGETRI computes the inverse of a matrix using the LU factorization
// computed by DGETRF.
//
// This method inverts U and then computes inv(A) by solving the system
// inv(A)*L = inv(U) for inv(A).
//
// Arguments
// =========
//
// N       (input) INTEGER
//         The order of the matrix A.  N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the factors L and U from the factorization
//         A = P*L*U as computed by DGETRF.
//         On exit, if INFO = 0, the inverse of the original matrix A.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// IPIV    (input) INTEGER array, dimension (N)
//         The pivot indices from DGETRF; for 1<=i<=N, row i of the
//         matrix was interchanged with row IPIV(i).
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO=0, then WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.  LWORK >= max(1,N).
//         For optimal performance LWORK >= N*NB, where NB is
//         the optimal blocksize returned by ILAENV.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//         > 0:  if INFO = i, U(i,i) is exactly zero; the matrix is
//               singular and its inverse could not be computed.
void DGETRI_F77(
	int * N,		// INTEGER
	double * A,		// DOUBLE PRECISION
	int * LDA,		// INTEGER
	unsigned int * IPIV,	// INTEGER IPIV( * )
	double * WORK,		// DOUBLE PRECISION WORK( * )
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGESV computes the solution to a real system of linear equations
//    A * X = B,
// where A is an N-by-N matrix and X and B are N-by-NRHS matrices.
//
// The LU decomposition with partial pivoting and row interchanges is
// used to factor A as
//    A = P * L * U,
// where P is a permutation matrix, L is unit lower triangular, and U is
// upper triangular.  The factored form of A is then used to solve the
// system of equations A * X = B.
//
// Arguments
// =========
//
// N       (input) INTEGER
//         The number of linear equations, i.e., the order of the
//         matrix A.  N >= 0.
//
// NRHS    (input) INTEGER
//         The number of right hand sides, i.e., the number of columns
//         of the matrix B.  NRHS >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the N-by-N coefficient matrix A.
//         On exit, the factors L and U from the factorization
//         A = P*L*U; the unit diagonal elements of L are not stored.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// IPIV    (output) INTEGER array, dimension (N)
//         The pivot indices that define the permutation matrix P;
//         row i of the matrix was interchanged with row IPIV(i).
//
// B       (input/output) DOUBLE PRECISION array, dimension (LDB,NRHS)
//         On entry, the N-by-NRHS matrix of right hand side matrix B.
//         On exit, if INFO = 0, the N-by-NRHS solution matrix X.
//
// LDB     (input) INTEGER
//         The leading dimension of the array B.  LDB >= max(1,N).
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//         > 0:  if INFO = i, U(i,i) is exactly zero.  The factorization
//               has been completed, but the factor U is exactly
//               singular, so the solution could not be computed.
void DGESV_F77(
	int * N,		// INTEGER
	int * NRHS,		// INTEGER
	double * A,		// DOUBLE PRECISION A( LDA, * )
	int * LDA,		// INTEGER
	unsigned int * IPIV,	// INTEGER IPIV( * )
	double * B,		// DOUBLE PRECISION B( LDB, * )
	int * LDB,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGESVX uses the LU factorization to compute the solution to a real
// system of linear equations
//    A * X = B,
// where A is an N-by-N matrix and X and B are N-by-NRHS matrices.
//
// Error bounds on the solution and a condition estimate are also
// provided.
//
// Description
// ===========
//
// The following steps are performed:
//
// 1. If FACT = 'E', real scaling factors are computed to equilibrate
//    the system:
//       TRANS = 'N':  diag(R)*A*diag(C)     *inv(diag(C))*X = diag(R)*B
//       TRANS = 'T': (diag(R)*A*diag(C))**T *inv(diag(R))*X = diag(C)*B
//       TRANS = 'C': (diag(R)*A*diag(C))**H *inv(diag(R))*X = diag(C)*B
//    Whether or not the system will be equilibrated depends on the
//    scaling of the matrix A, but if equilibration is used, A is
//    overwritten by diag(R)*A*diag(C) and B by diag(R)*B (if TRANS='N')
//    or diag(C)*B (if TRANS = 'T' or 'C').
//
// 2. If FACT = 'N' or 'E', the LU decomposition is used to factor the
//    matrix A (after equilibration if FACT = 'E') as
//       A = P * L * U,
//    where P is a permutation matrix, L is a unit lower triangular
//    matrix, and U is upper triangular.
//
// 3. If some U(i,i)=0, so that U is exactly singular, then the routine
//    returns with INFO = i. Otherwise, the factored form of A is used
//    to estimate the condition number of the matrix A.  If the
//    reciprocal of the condition number is less than machine precision,
//    INFO = N+1 is returned as a warning, but the routine still goes on
//    to solve for X and compute error bounds as described below.
//
// 4. The system of equations is solved for X using the factored form
//    of A.
//
// 5. Iterative refinement is applied to improve the computed solution
//    matrix and calculate error bounds and backward error estimates
//    for it.
//
// 6. If equilibration was used, the matrix X is premultiplied by
//    diag(C) (if TRANS = 'N') or diag(R) (if TRANS = 'T' or 'C') so
//    that it solves the original system before equilibration.
//
// Arguments
// =========
//
// FACT    (input) CHARACTER*1
//         Specifies whether or not the factored form of the matrix A is
//         supplied on entry, and if not, whether the matrix A should be
//         equilibrated before it is factored.
//         = 'F':  On entry, AF and IPIV contain the factored form of A.
//                 If EQUED is not 'N', the matrix A has been
//                 equilibrated with scaling factors given by R and C.
//                 A, AF, and IPIV are not modified.
//         = 'N':  The matrix A will be copied to AF and factored.
//         = 'E':  The matrix A will be equilibrated if necessary, then
//                 copied to AF and factored.
//
// TRANS   (input) CHARACTER*1
//         Specifies the form of the system of equations:
//         = 'N':  A * X = B     (No transpose)
//         = 'T':  A**T * X = B  (Transpose)
//         = 'C':  A**H * X = B  (Transpose)
//
// N       (input) INTEGER
//         The number of linear equations, i.e., the order of the
//         matrix A.  N >= 0.
//
// NRHS    (input) INTEGER
//         The number of right hand sides, i.e., the number of columns
//         of the matrices B and X.  NRHS >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the N-by-N matrix A.  If FACT = 'F' and EQUED is
//         not 'N', then A must have been equilibrated by the scaling
//         factors in R and/or C.  A is not modified if FACT = 'F' or
//         'N', or if FACT = 'E' and EQUED = 'N' on exit.
//
//         On exit, if EQUED .ne. 'N', A is scaled as follows:
//         EQUED = 'R':  A := diag(R) * A
//         EQUED = 'C':  A := A * diag(C)
//         EQUED = 'B':  A := diag(R) * A * diag(C).
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// AF      (input or output) DOUBLE PRECISION array, dimension (LDAF,N)
//         If FACT = 'F', then AF is an input argument and on entry
//         contains the factors L and U from the factorization
//         A = P*L*U as computed by DGETRF.  If EQUED .ne. 'N', then
//         AF is the factored form of the equilibrated matrix A.
//
//         If FACT = 'N', then AF is an output argument and on exit
//         returns the factors L and U from the factorization A = P*L*U
//         of the original matrix A.
//
//         If FACT = 'E', then AF is an output argument and on exit
//         returns the factors L and U from the factorization A = P*L*U
//         of the equilibrated matrix A (see the description of A for
//         the form of the equilibrated matrix).
//
// LDAF    (input) INTEGER
//         The leading dimension of the array AF.  LDAF >= max(1,N).
//
// IPIV    (input or output) INTEGER array, dimension (N)
//         If FACT = 'F', then IPIV is an input argument and on entry
//         contains the pivot indices from the factorization A = P*L*U
//         as computed by DGETRF; row i of the matrix was interchanged
//         with row IPIV(i).
//
//         If FACT = 'N', then IPIV is an output argument and on exit
//         contains the pivot indices from the factorization A = P*L*U
//         of the original matrix A.
//
//         If FACT = 'E', then IPIV is an output argument and on exit
//         contains the pivot indices from the factorization A = P*L*U
//         of the equilibrated matrix A.
//
// EQUED   (input or output) CHARACTER*1
//         Specifies the form of equilibration that was done.
//         = 'N':  No equilibration (always true if FACT = 'N').
//         = 'R':  Row equilibration, i.e., A has been premultiplied by
//                 diag(R).
//         = 'C':  Column equilibration, i.e., A has been postmultiplied
//                 by diag(C).
//         = 'B':  Both row and column equilibration, i.e., A has been
//                 replaced by diag(R) * A * diag(C).
//         EQUED is an input argument if FACT = 'F'; otherwise, it is an
//         output argument.
//
// R       (input or output) DOUBLE PRECISION array, dimension (N)
//         The row scale factors for A.  If EQUED = 'R' or 'B', A is
//         multiplied on the left by diag(R); if EQUED = 'N' or 'C', R
//         is not accessed.  R is an input argument if FACT = 'F';
//         otherwise, R is an output argument.  If FACT = 'F' and
//         EQUED = 'R' or 'B', each element of R must be positive.
//
// C       (input or output) DOUBLE PRECISION array, dimension (N)
//         The column scale factors for A.  If EQUED = 'C' or 'B', A is
//         multiplied on the right by diag(C); if EQUED = 'N' or 'R', C
//         is not accessed.  C is an input argument if FACT = 'F';
//         otherwise, C is an output argument.  If FACT = 'F' and
//         EQUED = 'C' or 'B', each element of C must be positive.
//
// B       (input/output) DOUBLE PRECISION array, dimension (LDB,NRHS)
//         On entry, the N-by-NRHS right hand side matrix B.
//         On exit,
//         if EQUED = 'N', B is not modified;
//         if TRANS = 'N' and EQUED = 'R' or 'B', B is overwritten by
//         diag(R)*B;
//         if TRANS = 'T' or 'C' and EQUED = 'C' or 'B', B is
//         overwritten by diag(C)*B.
//
// LDB     (input) INTEGER
//         The leading dimension of the array B.  LDB >= max(1,N).
//
// X       (output) DOUBLE PRECISION array, dimension (LDX,NRHS)
//         If INFO = 0 or INFO = N+1, the N-by-NRHS solution matrix X
//         to the original system of equations.  Note that A and B are
//         modified on exit if EQUED .ne. 'N', and the solution to the
//         equilibrated system is inv(diag(C))*X if TRANS = 'N' and
//         EQUED = 'C' or 'B', or inv(diag(R))*X if TRANS = 'T' or 'C'
//         and EQUED = 'R' or 'B'.
//
// LDX     (input) INTEGER
//         The leading dimension of the array X.  LDX >= max(1,N).
//
// RCOND   (output) DOUBLE PRECISION
//         The estimate of the reciprocal condition number of the matrix
//         A after equilibration (if done).  If RCOND is less than the
//         machine precision (in particular, if RCOND = 0), the matrix
//         is singular to working precision.  This condition is
//         indicated by a return code of INFO > 0.
//
// FERR    (output) DOUBLE PRECISION array, dimension (NRHS)
//         The estimated forward error bound for each solution vector
//         X(j) (the j-th column of the solution matrix X).
//         If XTRUE is the true solution corresponding to X(j), FERR(j)
//         is an estimated upper bound for the magnitude of the largest
//         element in (X(j) - XTRUE) divided by the magnitude of the
//         largest element in X(j).  The estimate is as reliable as
//         the estimate for RCOND, and is almost always a slight
//         overestimate of the true error.
//
// BERR    (output) DOUBLE PRECISION array, dimension (NRHS)
//         The componentwise relative backward error of each solution
//         vector X(j) (i.e., the smallest relative change in
//         any element of A or B that makes X(j) an exact solution).
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (4*N)
//         On exit, WORK(1) contains the reciprocal pivot growth
//         factor norm(A)/norm(U). The "max absolute element" norm is
//         used. If WORK(1) is much less than 1, then the stability
//         of the LU factorization of the (equilibrated) matrix A
//         could be poor. This also means that the solution X, condition
//         estimator RCOND, and forward error bound FERR could be
//         unreliable. If factorization fails with 0<INFO<=N, then
//         WORK(1) contains the reciprocal pivot growth factor for the
//         leading INFO columns of A.
//
// IWORK   (workspace) INTEGER array, dimension (N)
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//         > 0:  if INFO = i, and i is
//               <= N:  U(i,i) is exactly zero.  The factorization has
//                      been completed, but the factor U is exactly
//                      singular, so the solution and error bounds
//                      could not be computed. RCOND = 0 is returned.
//               = N+1: U is nonsingular, but RCOND is less than machine
//                      precision, meaning that the matrix is singular
//                      to working precision.  Nevertheless, the
//                      solution and error bounds are computed because
//                      there are a number of situations where the
//                      computed solution can be more accurate than the
//                      value of RCOND would suggest.
void DGESVX_F77(
	char * FACT,		// CHARACTER
	char * TRANS,		// CHARACTER
	int * N,		// INTEGER
	int * NRHS,		// INTEGER
	double * A,		// DOUBLE PRECISION A( LDA, * )
	int * LDA,		// INTEGER
	double * AF,		// DOUBLE PRECISION AF( LDAF, * )
	int * LDAF,		// INTEGER
	int * IPIV,		// INTEGER IPIV( * )
	char * EQUED,		// CHARACTER
	double * R,		// DOUBLE PRECISION R( * )
	double * C,		// DOUBLE PRECISION C( * )
	double * B,		// DOUBLE PRECISION B( LDB, * )
	int * LDB,		// INTEGER
	double * X,		// DOUBLE PRECISION X( LDX, * )
	int * LDX,		// INTEGER
	double * RCOND,		// DOUBLE PRECISION
	double * FERR,		// DOUBLE PRECISION FERR( * )
	double * BERR,		// DOUBLE PRECISION BERR( * )
	double * WORK,		// DOUBLE PRECISION WORK( * )
	int * IWORK,		// INTEGER IWORK( * )
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGEQRF computes a QR factorization of a real M-by-N matrix A:
// A = Q * R.
//
// Arguments
// =========
//
// M       (input) INTEGER
//         The number of rows of the matrix A.  M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the matrix A.  N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the M-by-N matrix A.
//         On exit, the elements on and above the diagonal of the array
//         contain the min(M,N)-by-N upper trapezoidal matrix R (R is
//         upper triangular if m >= n); the elements below the diagonal,
//         with the array TAU, represent the orthogonal matrix Q as a
//         product of min(m,n) elementary reflectors (see Further
//         Details).
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,M).
//
// TAU     (output) DOUBLE PRECISION array, dimension (min(M,N))
//         The scalar factors of the elementary reflectors (see Further
//         Details).
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.  LWORK >= max(1,N).
//         For optimum performance LWORK >= N*NB, where NB is
//         the optimal blocksize.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//
// Further Details
// ===============
//
// The matrix Q is represented as a product of elementary reflectors
//
//    Q = H(1) H(2) . . . H(k), where k = min(m,n).
//
// Each H(i) has the form
//
//    H(i) = I - tau * v * v'
//
// where tau is a real scalar, and v is a real vector with
// v(1:i-1) = 0 and v(i) = 1; v(i+1:m) is stored on exit in A(i+1:m,i),
// and tau in TAU(i).
//
// =====================================================================
void DGEQRF_F77(
	int * M,		// INTEGER
	int * N,		// INTEGER
	double * A,		// DOUBLE PRECISION A ( LDA, * )
	int * LDA,		// INTEGER
	double * TAU,		// DOUBLE PRECISION TAU ( * )
	double * WORK,		// DOUBLE PRECISION WORK ( * )
	int * LWORK,		// INTEGER LWORK ( >=N*NB )
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGEQP3 computes a QR factorization with column pivoting of a
// matrix A:  A*P = Q*R  using Level 3 BLAS.
//
// Arguments
// =========
//
// M       (input) INTEGER
//         The number of rows of the matrix A. M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the matrix A.  N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the M-by-N matrix A.
//         On exit, the upper triangle of the array contains the
//         min(M,N)-by-N upper trapezoidal matrix R; the elements below
//         the diagonal, together with the array TAU, represent the
//         orthogonal matrix Q as a product of min(M,N) elementary
//         reflectors.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A. LDA >= max(1,M).
//
// JPVT    (input/output) INTEGER array, dimension (N)
//         On entry, if JPVT(J).ne.0, the J-th column of A is permuted
//         to the front of A*P (a leading column); if JPVT(J)=0,
//         the J-th column of A is a free column.
//         On exit, if JPVT(J)=K, then the J-th column of A*P was the
//         the K-th column of A.
//
// TAU     (output) DOUBLE PRECISION array, dimension (min(M,N))
//         The scalar factors of the elementary reflectors.
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO=0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK. LWORK >= 3*N+1.
//         For optimal performance LWORK >= 2*N+( N+1 )*NB, where NB
//         is the optimal blocksize.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0: successful exit.
//         < 0: if INFO = -i, the i-th argument had an illegal value.
//
// Further Details
// ===============
//
// The matrix Q is represented as a product of elementary reflectors
//
//    Q = H(1) H(2) . . . H(k), where k = min(m,n).
//
// Each H(i) has the form
//
//    H(i) = I - tau * v * v'
//
// where tau is a real/complex scalar, and v is a real/complex vector
// with v(1:i-1) = 0 and v(i) = 1; v(i+1:m) is stored on exit in
// A(i+1:m,i), and tau in TAU(i).
//
// Based on contributions by
//   G. Quintana-Orti, Depto. de Informatica, Universidad Jaime I, Spain
//   X. Sun, Computer Science Dept., Duke University, USA
//
// =====================================================================
void DGEQP3_F77(
	int * M,		// INTEGER
	int * N,		// INTEGER
	double * A,		// DOUBLE PRECISION ( LDA, * )
	int * LDA,		// INTEGER
	int * JPVT,		// INTEGER ( * )
	double * TAU,		// DOUBLE PRECISION ( * )
	double * WORK,		// DOUBLE PRECISION ( * )
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DORGQR generates an M-by-N real matrix Q with orthonormal columns,
// which is defined as the first N columns of a product of K elementary
// reflectors of order M
//
//       Q  =  H(1) H(2) . . . H(k)
//
// as returned by DGEQRF.
//
// Arguments
// =========
//
// M       (input) INTEGER
//         The number of rows of the matrix Q. M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the matrix Q. M >= N >= 0.
//
// K       (input) INTEGER
//         The number of elementary reflectors whose product defines the
//         matrix Q. N >= K >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the i-th column must contain the vector which
//         defines the elementary reflector H(i), for i = 1,2,...,k, as
//         returned by DGEQRF in the first k columns of its array
//         argument A.
//         On exit, the M-by-N matrix Q.
//
// LDA     (input) INTEGER
//         The first dimension of the array A. LDA >= max(1,M).
//
// TAU     (input) DOUBLE PRECISION array, dimension (K)
//         TAU(i) must contain the scalar factor of the elementary
//         reflector H(i), as returned by DGEQRF.
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK. LWORK >= max(1,N).
//         For optimum performance LWORK >= N*NB, where NB is the
//         optimal blocksize.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument has an illegal value
//
// =====================================================================
void DORGQR_F77(
	int * M,		// INTEGER
	int * N,		// INTEGER
	int * K,		// INTEGER
	double * A,		// DOUBLE PRECISION ( LDA, * )
	int * LDA,		// INTEGER
	double * TAU,		// DOUBLE PRECISION ( * )
	double * WORK,		// DOUBLE PRECISION ( * )
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DORMQR overwrites the general real M-by-N matrix C with
//
//                 SIDE = 'L'     SIDE = 'R'
// TRANS = 'N':      Q * C          C * Q
// TRANS = 'T':      Q**T * C       C * Q**T
//
// where Q is a real orthogonal matrix defined as the product of k
// elementary reflectors
//
//       Q = H(1) H(2) . . . H(k)
//
// as returned by DGEQRF. Q is of order M if SIDE = 'L' and of order N
// if SIDE = 'R'.
//
// Arguments
// =========
//
// SIDE    (input) CHARACTER*1
//         = 'L': apply Q or Q**T from the Left;
//         = 'R': apply Q or Q**T from the Right.
//
// TRANS   (input) CHARACTER*1
//         = 'N':  No transpose, apply Q;
//         = 'T':  Transpose, apply Q**T.
//
// M       (input) INTEGER
//         The number of rows of the matrix C. M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the matrix C. N >= 0.
//
// K       (input) INTEGER
//         The number of elementary reflectors whose product defines
//         the matrix Q.
//         If SIDE = 'L', M >= K >= 0;
//         if SIDE = 'R', N >= K >= 0.
//
// A       (input) DOUBLE PRECISION array, dimension (LDA,K)
//         The i-th column must contain the vector which defines the
//         elementary reflector H(i), for i = 1,2,...,k, as returned by
//         DGEQRF in the first k columns of its array argument A.
//         A is modified by the routine but restored on exit.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.
//         If SIDE = 'L', LDA >= max(1,M);
//         if SIDE = 'R', LDA >= max(1,N).
//
// TAU     (input) DOUBLE PRECISION array, dimension (K)
//         TAU(i) must contain the scalar factor of the elementary
//         reflector H(i), as returned by DGEQRF.
//
// C       (input/output) DOUBLE PRECISION array, dimension (LDC,N)
//         On entry, the M-by-N matrix C.
//         On exit, C is overwritten by Q*C or Q**T*C or C*Q**T or C*Q.
//
// LDC     (input) INTEGER
//         The leading dimension of the array C. LDC >= max(1,M).
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.
//         If SIDE = 'L', LWORK >= max(1,N);
//         if SIDE = 'R', LWORK >= max(1,M).
//         For optimum performance LWORK >= N*NB if SIDE = 'L', and
//         LWORK >= M*NB if SIDE = 'R', where NB is the optimal
//         blocksize.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//
// =====================================================================
void DORMQR_F77(
	char * SIDE,		// CHARACTER
	char * TRANS,		// CHARACTER
	int * M,		// INTEGER
	int * N,		// INTEGER
	int * K,		// INTEGER
	double * A,		// DOUBLE PRECISION ( LDA, * )
	int * LDA,		// INTEGER
	double * TAU,		// DOUBLE PRECISION ( * )
	double * C,		// DOUBLE PRECISION ( LDC, * )
	int * LDC,		// INTEGER
	double * WORK,		// DOUBLE PRECISION ( * )
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DGELSY computes the minimum-norm solution to a real linear least
// squares problem:
//     minimize || A * X - B ||
// using a complete orthogonal factorization of A.  A is an M-by-N
// matrix which may be rank-deficient.
//
// Several right hand side vectors b and solution vectors x can be
// handled in a single call; they are stored as the columns of the
// M-by-NRHS right hand side matrix B and the N-by-NRHS solution
// matrix X.
//
// The routine first computes a QR factorization with column pivoting:
//     A * P = Q * [ R11 R12 ]
//                 [  0  R22 ]
// with R11 defined as the largest leading submatrix whose estimated
// condition number is less than 1/RCOND.  The order of R11, RANK,
// is the effective rank of A.
//
// Then, R22 is considered to be negligible, and R12 is annihilated
// by orthogonal transformations from the right, arriving at the
// complete orthogonal factorization:
//    A * P = Q * [ T11 0 ] * Z
//                [  0  0 ]
// The minimum-norm solution is then
//    X = P * Z' [ inv(T11)*Q1'*B ]
//               [        0       ]
// where Q1 consists of the first RANK columns of Q.
//
// This routine is basically identical to the original xGELSX except
// three differences:
//   o The call to the subroutine xGEQPF has been substituted by the
//     the call to the subroutine xGEQP3. This subroutine is a Blas-3
//     version of the QR factorization with column pivoting.
//   o Matrix B (the right hand side) is updated with Blas-3.
//   o The permutation of matrix B (the right hand side) is faster and
//     more simple.
//
// Arguments
// =========
//
// M       (input) INTEGER
//         The number of rows of the matrix A.  M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the matrix A.  N >= 0.
//
// NRHS    (input) INTEGER
//         The number of right hand sides, i.e., the number of
//         columns of matrices B and X. NRHS >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the M-by-N matrix A.
//         On exit, A has been overwritten by details of its
//         complete orthogonal factorization.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,M).
//
// B       (input/output) DOUBLE PRECISION array, dimension (LDB,NRHS)
//         On entry, the M-by-NRHS right hand side matrix B.
//         On exit, the N-by-NRHS solution matrix X.
//
// LDB     (input) INTEGER
//         The leading dimension of the array B. LDB >= max(1,M,N).
//
// JPVT    (input/output) INTEGER array, dimension (N)
//         On entry, if JPVT(i) .ne. 0, the i-th column of A is permuted
//         to the front of AP, otherwise column i is a free column.
//         On exit, if JPVT(i) = k, then the i-th column of AP
//         was the k-th column of A.
//
// RCOND   (input) DOUBLE PRECISION
//         RCOND is used to determine the effective rank of A, which
//         is defined as the order of the largest leading triangular
//         submatrix R11 in the QR factorization with pivoting of A,
//         whose estimated condition number < 1/RCOND.
//
// RANK    (output) INTEGER
//         The effective rank of A, i.e., the order of the submatrix
//         R11.  This is the same as the order of the submatrix T11
//         in the complete orthogonal factorization of A.
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.
//         The unblocked strategy requires that:
//            LWORK >= MAX( MN+3*N+1, 2*MN+NRHS ),
//         where MN = min( M, N ).
//         The block algorithm requires that:
//            LWORK >= MAX( MN+2*N+NB*(N+1), 2*MN+NB*NRHS ),
//         where NB is an upper bound on the blocksize returned
//         by ILAENV for the routines DGEQP3, DTZRZF, STZRQF, DORMQR,
//         and DORMRZ.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0: successful exit
//         < 0: If INFO = -i, the i-th argument had an illegal value.
//
// Further Details
// ===============
//
// Based on contributions by
//   A. Petitet, Computer Science Dept., Univ. of Tenn., Knoxville, USA
//   E. Quintana-Orti, Depto. de Informatica, Universidad Jaime I, Spain
//   G. Quintana-Orti, Depto. de Informatica, Universidad Jaime I, Spain
//
// =====================================================================
#if 0
void DGELSY_F77(
	int * M,		// INTEGER
	int * N,		// INTEGER
	int * NRHS,		// INTEGER
	double * A,		// DOUBLE PRECISION ( LDA, * )
	int * LDA,		// INTEGER
	double * B,		// DOUBLE PRECISION ( LDB, * )
	int * LDB,		// INTEGER
	int * JPVT,		// INTEGER ( * )
	double * RCOND,		// DOUBLE PRECISION
	int * RANK,		// INTEGER
	double * WORK,		// DOUBLE PRECISION
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);
#endif

// Purpose
// =======
//
// DGELS solves overdetermined or underdetermined real linear systems
// involving an M-by-N matrix A, or its transpose, using a QR or LQ
// factorization of A.  It is assumed that A has full rank.
//
// The following options are provided:
//
// 1. If TRANS = 'N' and m >= n:  find the least squares solution of
//    an overdetermined system, i.e., solve the least squares problem
//                 minimize || B - A*X ||.
//
// 2. If TRANS = 'N' and m < n:  find the minimum norm solution of
//    an underdetermined system A * X = B.
//
// 3. If TRANS = 'T' and m >= n:  find the minimum norm solution of
//    an undetermined system A**T * X = B.
//
// 4. If TRANS = 'T' and m < n:  find the least squares solution of
//    an overdetermined system, i.e., solve the least squares problem
//                 minimize || B - A**T * X ||.
//
// Several right hand side vectors b and solution vectors x can be
// handled in a single call; they are stored as the columns of the
// M-by-NRHS right hand side matrix B and the N-by-NRHS solution
// matrix X.
//
// Arguments
// =========
//
// TRANS   (input) CHARACTER*1
//         = 'N': the linear system involves A;
//         = 'T': the linear system involves A**T.
//
// M       (input) INTEGER
//         The number of rows of the matrix A.  M >= 0.
//
// N       (input) INTEGER
//         The number of columns of the matrix A.  N >= 0.
//
// NRHS    (input) INTEGER
//         The number of right hand sides, i.e., the number of
//         columns of the matrices B and X. NRHS >=0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the M-by-N matrix A.
//         On exit,
//           if M >= N, A is overwritten by details of its QR
//                      factorization as returned by DGEQRF;
//           if M <  N, A is overwritten by details of its LQ
//                      factorization as returned by DGELQF.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,M).
//
// B       (input/output) DOUBLE PRECISION array, dimension (LDB,NRHS)
//         On entry, the matrix B of right hand side vectors, stored
//         columnwise; B is M-by-NRHS if TRANS = 'N', or N-by-NRHS
//         if TRANS = 'T'.
//         On exit, if INFO = 0, B is overwritten by the solution
//         vectors, stored columnwise:
//         if TRANS = 'N' and m >= n, rows 1 to n of B contain the least
//         squares solution vectors; the residual sum of squares for the
//         solution in each column is given by the sum of squares of
//         elements N+1 to M in that column;
//         if TRANS = 'N' and m < n, rows 1 to N of B contain the
//         minimum norm solution vectors;
//         if TRANS = 'T' and m >= n, rows 1 to M of B contain the
//         minimum norm solution vectors;
//         if TRANS = 'T' and m < n, rows 1 to M of B contain the
//         least squares solution vectors; the residual sum of squares
//         for the solution in each column is given by the sum of
//         squares of elements M+1 to N in that column.
//
// LDB     (input) INTEGER
//         The leading dimension of the array B. LDB >= MAX(1,M,N).
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.
//         LWORK >= max( 1, MN + max( MN, NRHS ) ).
//         For optimal performance,
//         LWORK >= max( 1, MN + max( MN, NRHS )*NB ).
//         where MN = min(M,N) and NB is the optimum block size.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//         > 0:  if INFO =  i, the i-th diagonal element of the
//               triangular factor of A is zero, so that A does not have
//               full rank; the least squares solution could not be
//               computed.
//
// =====================================================================
void DGELS_F77(
	char * TRANS,		// CHARACTER
	int * M,		// INTEGER
	int * N,		// INTEGER
	int * NRHS,		// INTEGER
	double * A,		// DOUBLE PRECISION ( LDA, * )
	int * LDA,		// INTEGER
	double * B,		// DOUBLE PRECISION ( LDB, * )
	int * LDB,		// INTEGER
	double * WORK,		// DOUBLE PRECISION ( * )
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);

void DGELSS_F77(
	int * M,		// INTEGER
	int * N,		// INTEGER
	int * NRHS,		// INTEGER
	double * A,		// DOUBLE PRECISION ( LDA, * )
	int * LDA,		// INTEGER
	double * B,		// DOUBLE PRECISION ( LDB, * )
	int * LDB,		// INTEGER
	double * S,		// DOUBLE PRECISION ( MIN(N,M), * )
	double * RCOND,		// DOUBLE PRECISION
	int * RANK,		// INTEGER
	double * WORK,		// DOUBLE PRECISION ( * )
	int * LWORK,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// ZGEES computes for an N-by-N complex nonsymmetric matrix A, the
// eigenvalues, the Schur form T, and, optionally, the matrix of Schur
// vectors Z.  This gives the Schur factorization A = Z*T*(Z**H).
//
// Optionally, it also orders the eigenvalues on the diagonal of the
// Schur form so that selected eigenvalues are at the top left.
// The leading columns of Z then form an orthonormal basis for the
// invariant subspace corresponding to the selected eigenvalues.
//
// A complex matrix is in Schur form if it is upper triangular.
//
// Arguments
// =========
//
// JOBVS   (input) CHARACTER*1
//         = 'N': Schur vectors are not computed;
//         = 'V': Schur vectors are computed.
//
// SORT    (input) CHARACTER*1
//         Specifies whether or not to order the eigenvalues on the
//         diagonal of the Schur form.
//         = 'N': Eigenvalues are not ordered:
//         = 'S': Eigenvalues are ordered (see SELECT).
//
// SELECT  (external procedure) LOGICAL FUNCTION of one COMPLEX*16 argument
//         SELECT must be declared EXTERNAL in the calling subroutine.
//         If SORT = 'S', SELECT is used to select eigenvalues to order
//         to the top left of the Schur form.
//         IF SORT = 'N', SELECT is not referenced.
//         The eigenvalue W(j) is selected if SELECT(W(j)) is true.
//
// N       (input) INTEGER
//         The order of the matrix A. N >= 0.
//
// A       (input/output) COMPLEX*16 array, dimension (LDA,N)
//         On entry, the N-by-N matrix A.
//         On exit, A has been overwritten by its Schur form T.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// SDIM    (output) INTEGER
//         If SORT = 'N', SDIM = 0.
//         If SORT = 'S', SDIM = number of eigenvalues for which
//                        SELECT is true.
//
// W       (output) COMPLEX*16 array, dimension (N)
//         W contains the computed eigenvalues, in the same order that
//         they appear on the diagonal of the output Schur form T.
//
// VS      (output) COMPLEX*16 array, dimension (LDVS,N)
//         If JOBVS = 'V', VS contains the unitary matrix Z of Schur
//         vectors.
//         If JOBVS = 'N', VS is not referenced.
//
// LDVS    (input) INTEGER
//         The leading dimension of the array VS.  LDVS >= 1; if
//         JOBVS = 'V', LDVS >= N.
//
// WORK    (workspace/output) COMPLEX*16 array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.  LWORK >= max(1,2*N).
//         For good performance, LWORK must generally be larger.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// RWORK   (workspace) DOUBLE PRECISION array, dimension (N)
//
// BWORK   (workspace) LOGICAL array, dimension (N)
//         Not referenced if SORT = 'N'.
//
// INFO    (output) INTEGER
//         = 0: successful exit
//         < 0: if INFO = -i, the i-th argument had an illegal value.
//         > 0: if INFO = i, and i is
//              <= N:  the QR algorithm failed to compute all the
//                     eigenvalues; elements 1:ILO-1 and i+1:N of W
//                     contain those eigenvalues which have converged;
//                     if JOBVS = 'V', VS contains the matrix which
//                     reduces A to its partially converged Schur form.
//              = N+1: the eigenvalues could not be reordered because
//                     some eigenvalues were too close to separate (the
//                     problem is very ill-conditioned);
//              = N+2: after reordering, roundoff changed values of
//                     some complex eigenvalues so that leading
//                     eigenvalues in the Schur form no longer satisfy
//                     SELECT = .TRUE..  This could also be caused by
//                     underflow due to scaling.
//
// =====================================================================

// TODO: Passt std::complex< double > immer?
//typedef std::complex< double > COMPLEX16;

void ZGEES_F77(
	char * JOBVS,		// CHARACTER
	char * SORT,		// CHARACTER
	int (*)(COMPLEX16),	// LOGICAL(4)
	int * N,		// INTEGER
	COMPLEX16 * A,		// COMPLEX*16 A( LDA, * )
	int * LDA,		// INTEGER
	int * SDIM,		// INTEGER
	COMPLEX16 * W,		// COMPLEX16 W( * )
	COMPLEX16 * VS,		// COMPLEX16 VS( LDVS, * )
	int * LDVS,		// INTEGER
	COMPLEX16 * WORK,	// COMPLEX16 WORK( * ),
	int * LWORK,		// INTEGER
	double * RWORK,		// DOUBLE PRECISIOM RWORK( * )
	int * BWORK,		// LOGICAL(4)( * )
	int * INFO
	);

// Purpose
// =======
//
// DPOTRF computes the Cholesky factorization of a real symmetric
// positive definite matrix A.
//
// The factorization has the form
//    A = U**T * U,  if UPLO = 'U', or
//    A = L  * L**T,  if UPLO = 'L',
// where U is an upper triangular matrix and L is lower triangular.
//
// This is the block version of the algorithm, calling Level 3 BLAS.
//
// Arguments
// =========
//
// UPLO    (input) CHARACTER*1
//         = 'U':  Upper triangle of A is stored;
//         = 'L':  Lower triangle of A is stored.
//
// N       (input) INTEGER
//         The order of the matrix A.  N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the symmetric matrix A.  If UPLO = 'U', the leading
//         N-by-N upper triangular part of A contains the upper
//         triangular part of the matrix A, and the strictly lower
//         triangular part of A is not referenced.  If UPLO = 'L', the
//         leading N-by-N lower triangular part of A contains the lower
//         triangular part of the matrix A, and the strictly upper
//         triangular part of A is not referenced.
//
//         On exit, if INFO = 0, the factor U or L from the Cholesky
//         factorization A = U**T*U or A = L*L**T.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//         > 0:  if INFO = i, the leading minor of order i is not
//               positive definite, and the factorization could not be
//               completed.
//
// =====================================================================
void DPOTRF_F77(
	char * UPLO,		// CHARACTER
	int * N,		// INTEGER
	double * A,		// DOUBLE PRECISION ( LDA, * )
	int * LDA,		// INTEGER
	int * INFO		// INTEGER
	);

// Purpose
// =======
//
// DSYTRD reduces a real symmetric matrix A to real symmetric
// tridiagonal form T by an orthogonal similarity transformation:
// Q**T * A * Q = T.
//
// Arguments
// =========
//
// UPLO    (input) CHARACTER*1
//         = 'U':  Upper triangle of A is stored;
//         = 'L':  Lower triangle of A is stored.
//
// N       (input) INTEGER
//         The order of the matrix A.  N >= 0.
//
// A       (input/output) DOUBLE PRECISION array, dimension (LDA,N)
//         On entry, the symmetric matrix A.  If UPLO = 'U', the leading
//         N-by-N upper triangular part of A contains the upper
//         triangular part of the matrix A, and the strictly lower
//         triangular part of A is not referenced.  If UPLO = 'L', the
//         leading N-by-N lower triangular part of A contains the lower
//         triangular part of the matrix A, and the strictly upper
//         triangular part of A is not referenced.
//         On exit, if UPLO = 'U', the diagonal and first superdiagonal
//         of A are overwritten by the corresponding elements of the
//         tridiagonal matrix T, and the elements above the first
//         superdiagonal, with the array TAU, represent the orthogonal
//         matrix Q as a product of elementary reflectors; if UPLO
//         = 'L', the diagonal and first subdiagonal of A are over-
//         written by the corresponding elements of the tridiagonal
//         matrix T, and the elements below the first subdiagonal, with
//         the array TAU, represent the orthogonal matrix Q as a product
//         of elementary reflectors. See Further Details.
//
// LDA     (input) INTEGER
//         The leading dimension of the array A.  LDA >= max(1,N).
//
// D       (output) DOUBLE PRECISION array, dimension (N)
//         The diagonal elements of the tridiagonal matrix T:
//         D(i) = A(i,i).
//
// E       (output) DOUBLE PRECISION array, dimension (N-1)
//         The off-diagonal elements of the tridiagonal matrix T:
//         E(i) = A(i,i+1) if UPLO = 'U', E(i) = A(i+1,i) if UPLO = 'L'.
//
// TAU     (output) DOUBLE PRECISION array, dimension (N-1)
//         The scalar factors of the elementary reflectors (see Further
//         Details).
//
// WORK    (workspace/output) DOUBLE PRECISION array, dimension (MAX(1,LWORK))
//         On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
//
// LWORK   (input) INTEGER
//         The dimension of the array WORK.  LWORK >= 1.
//         For optimum performance LWORK >= N*NB, where NB is the
//         optimal blocksize.
//
//         If LWORK = -1, then a workspace query is assumed; the routine
//         only calculates the optimal size of the WORK array, returns
//         this value as the first entry of the WORK array, and no error
//         message related to LWORK is issued by XERBLA.
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
// =====================================================================
void DSYTRD_F77(
	char * UPLO,
	int * N,
	double * A,
	int * LDA,
	double * D,
	double * E,
	double * TAU,
	double * WORK,
	int * LWORK,
	int * INFO
	);

// Purpose
// =======
//
// DSTEBZ computes the eigenvalues of a symmetric tridiagonal
// matrix T.  The user may ask for all eigenvalues, all eigenvalues
// in the half-open interval (VL, VU], or the IL-th through IU-th
// eigenvalues.
//
// To avoid overflow, the matrix must be scaled so that its
// largest element is no greater than overflow**(1/2) *
// underflow**(1/4) in absolute value, and for greatest
// accuracy, it should not be much smaller than that.
//
// See W. Kahan "Accurate Eigenvalues of a Symmetric Tridiagonal
// Matrix", Report CS41, Computer Science Dept., Stanford
// University, July 21, 1966.
//
// Arguments
// =========
//
// RANGE   (input) CHARACTER*1
//         = 'A': ("All")   all eigenvalues will be found.
//         = 'V': ("Value") all eigenvalues in the half-open interval
//                          (VL, VU] will be found.
//         = 'I': ("Index") the IL-th through IU-th eigenvalues (of the
//                          entire matrix) will be found.
//
// ORDER   (input) CHARACTER*1
//         = 'B': ("By Block") the eigenvalues will be grouped by
//                             split-off block (see IBLOCK, ISPLIT) and
//                             ordered from smallest to largest within
//                             the block.
//         = 'E': ("Entire matrix")
//                             the eigenvalues for the entire matrix
//                             will be ordered from smallest to
//                             largest.
//
// N       (input) INTEGER
//         The order of the tridiagonal matrix T.  N >= 0.
//
// VL      (input) DOUBLE PRECISION
// VU      (input) DOUBLE PRECISION
//         If RANGE='V', the lower and upper bounds of the interval to
//         be searched for eigenvalues.  Eigenvalues less than or equal
//         to VL, or greater than VU, will not be returned.  VL < VU.
//         Not referenced if RANGE = 'A' or 'I'.
//
// IL      (input) INTEGER
// IU      (input) INTEGER
//         If RANGE='I', the indices (in ascending order) of the
//         smallest and largest eigenvalues to be returned.
//         1 <= IL <= IU <= N, if N > 0; IL = 1 and IU = 0 if N = 0.
//         Not referenced if RANGE = 'A' or 'V'.
//
// ABSTOL  (input) DOUBLE PRECISION
//         The absolute tolerance for the eigenvalues.  An eigenvalue
//         (or cluster) is considered to be located if it has been
//         determined to lie in an interval whose width is ABSTOL or
//         less.  If ABSTOL is less than or equal to zero, then ULP*|T|
//         will be used, where |T| means the 1-norm of T.
//
//         Eigenvalues will be computed most accurately when ABSTOL is
//         set to twice the underflow threshold 2*DLAMCH('S'), not zero.
//
// D       (input) DOUBLE PRECISION array, dimension (N)
//         The n diagonal elements of the tridiagonal matrix T.
//
// E       (input) DOUBLE PRECISION array, dimension (N-1)
//         The (n-1) off-diagonal elements of the tridiagonal matrix T.
//
// M       (output) INTEGER
//         The actual number of eigenvalues found. 0 <= M <= N.
//         (See also the description of INFO=2,3.)
//
// NSPLIT  (output) INTEGER
//         The number of diagonal blocks in the matrix T.
//         1 <= NSPLIT <= N.
//
// W       (output) DOUBLE PRECISION array, dimension (N)
//         On exit, the first M elements of W will contain the
//         eigenvalues.  (DSTEBZ may use the remaining N-M elements as
//         workspace.)
//
// IBLOCK  (output) INTEGER array, dimension (N)
//         At each row/column j where E(j) is zero or small, the
//         matrix T is considered to split into a block diagonal
//         matrix.  On exit, if INFO = 0, IBLOCK(i) specifies to which
//         block (from 1 to the number of blocks) the eigenvalue W(i)
//         belongs.  (DSTEBZ may use the remaining N-M elements as
//         workspace.)
//
// ISPLIT  (output) INTEGER array, dimension (N)
//         The splitting points, at which T breaks up into submatrices.
//         The first submatrix consists of rows/columns 1 to ISPLIT(1),
//         the second of rows/columns ISPLIT(1)+1 through ISPLIT(2),
//         etc., and the NSPLIT-th consists of rows/columns
//         ISPLIT(NSPLIT-1)+1 through ISPLIT(NSPLIT)=N.
//         (Only the first NSPLIT elements will actually be used, but
//         since the user cannot know a priori what value NSPLIT will
//         have, N words must be reserved for ISPLIT.)
//
// WORK    (workspace) DOUBLE PRECISION array, dimension (4*N)
//
// IWORK   (workspace) INTEGER array, dimension (3*N)
//
// INFO    (output) INTEGER
//         = 0:  successful exit
//         < 0:  if INFO = -i, the i-th argument had an illegal value
//         > 0:  some or all of the eigenvalues failed to converge or
//               were not computed:
//               =1 or 3: Bisection failed to converge for some
//                       eigenvalues; these eigenvalues are flagged by a
//                       negative block number.  The effect is that the
//                       eigenvalues may not be as accurate as the
//                       absolute and relative tolerances.  This is
//                       generally caused by unexpectedly inaccurate
//                       arithmetic.
//               =2 or 3: RANGE='I' only: Not all of the eigenvalues
//                       IL:IU were found.
//                       Effect: M < IU+1-IL
//                       Cause:  non-monotonic arithmetic, causing the
//                               Sturm sequence to be non-monotonic.
//                       Cure:   recalculate, using RANGE='A', and pick
//                               out eigenvalues IL:IU.  In some cases,
//                               increasing the PARAMETER "FUDGE" may
//                               make things work.
//               = 4:    RANGE='I', and the Gershgorin interval
//                       initially used was too small.  No eigenvalues
//                       were computed.
//                       Probable cause: your machine has sloppy
//                                       floating-point arithmetic.
//                       Cure: Increase the PARAMETER "FUDGE",
//                             recompile, and try again.
// =====================================================================
void DSTEBZ_F77(
	char * RANGE,
	char * ORDER,
	int * N,
	double * VL,
	double * VU,
	int * IL,
	int * IU,
	double * ABSTOL,
	double * D,
	double * E,
	int * M,
	int * NSPLIT,
	double * W,
	int * IBLOCK,
	int * ISPLIT,
	double * WORK,
	int * IWORK,
	int * INFO
	);

} // extern "C"

struct dgesvd_opt_t
{
	char JOBU, JOBVT;
	int M, N;
	double * A;
	int LDA;
	double * S;
	double * U;
	int LDU;
	double * VT;
	int LDVT;
	int INFO;
};

struct dgeevx_opt_t
{
	char BALANC, JOBVL, JOBVR, SENSE;
	int N;
	double * A;
	int LDA;
	double * WR, * WI;
	double * VL;
	int LDVL;
	double * VR;
	int LDVR, ILO, IHI;
	double * SCALE;
	double ABNRM;
	double * RCONDE, * RCONDV, * WORK;
	int LWORK;
	int * IWORK;
	int INFO;
};

namespace flux {
namespace la {
namespace lapack {

#define MIN2(a,b)	((a)<=(b)?(a):(b))
#define MAX2(a,b)	((a)>=(b)?(a):(b))

void svd(MMatrix & A, MMatrix & U, MVector & s, MMatrix & VT)
{
	double * WORK;
	int LWORK = -1;
	double ws_query;

	dgesvd_opt_t o;
	o.JOBU = 'A';
	o.JOBVT = 'A';
	o.M = A.rows();
	o.N = A.cols();
	o.A = A;
	o.LDA = o.M;
	o.S = s;
	o.U = U;
	o.LDU = o.M;
	o.VT = VT;
	o.LDVT = o.N;

	// Fehlbedienung?
	fASSERT( VT.rows()==size_t(o.N) && VT.cols()==size_t(o.N) );
	fASSERT( U.rows()==size_t(o.M) && U.cols()==size_t(o.M) );
	fASSERT( s.dim()==size_t(MIN2(o.M,o.N)) );

	// optimale größe von WORK bestimmen (LWORK==-1)
	DGESVD_F77(&o.JOBU,
		&o.JOBVT,
		&o.M,
		&o.N,
		 o.A,
		&o.LDA,
		 o.S,
		 o.U,
		&o.LDU,
		 o.VT,
		&o.LDVT,
		&ws_query,
		&LWORK,
		&o.INFO
		);

	if (o.INFO != 0)
		return;

	// WORK allokieren und SVD berechnen
	LWORK = (int)ws_query;
	WORK = new double[LWORK];
	DGESVD_F77(&o.JOBU,
		&o.JOBVT,
		&o.M,
		&o.N,
		 o.A,
		&o.LDA,
		 o.S,
		 o.U,
		&o.LDU,
		 o.VT,
		&o.LDVT,
		 WORK,
		&LWORK,
		&o.INFO
		);
	delete[] WORK;
}

void svd(MMatrix & A, MVector & s)
{
	// FORTRAN-KACK!
	// JOBU='N', JOBVT='N' führt nur zu Segfaults :-(
	// Angeblich werden U und VT dann nicht verwendet -- das ist aber
	// nicht so!
	MMatrix U(A.rows(),A.rows());
	MMatrix VT(A.cols(),A.cols());
	svd(A,U,s,VT);
}

bool LUfactor(MMatrix & A, PMatrix & P)
{
	int M = A.rows();
	int N = A.cols();
	int INFO = 0;

	fASSERT( P.dim() == size_t(MIN2(M,N)) );
	DGETRF_F77(&M,&N,A,&M,P,&INFO);
	fASSERT( INFO >= 0 ); // ansonsten illegal argument

	// Fortran-Indizierung korrigieren 1..MIN(M,N) -> 0..MIN(M,N)-1
	for (int i=0; i<MIN2(M,N); i++)
		P(i) -= 1;

	return INFO == 0;
}

bool LUsolve(MMatrix & LU, PMatrix & P, MVector & b)
{
	int N = LU.rows();
	int NRHS = 1;
	int INFO = 0;

	// hier muss leider wieder die Indizierung der Permutation
	// angepasst werden ...
	for (int i=0; i<N; i++) P(i) += 1;
	DGETRS_F77(const_cast<char*>("N"),&N,&NRHS,LU,&N,P,b,&N,&INFO);
	for (int i=0; i<N; i++) P(i) -= 1;
	return INFO == 0;
}

bool LUsolve(MMatrix & LU, PMatrix & P, MMatrix & B)
{
	int N = LU.rows();
	int NRHS = B.cols();
	int INFO = 0;

	// hier muss leider wieder die Indizierung der Permutation
	// angepasst werden ...
	for (int i=0; i<N; i++) P(i) += 1;
	DGETRS_F77(const_cast<char*>("N"),&N,&NRHS,LU,&N,P,B,&N,&INFO);
	for (int i=0; i<N; i++) P(i) -= 1;
	return INFO == 0;
}

bool invert(MMatrix & A)
{
	int M = A.rows();
	int N = A.cols();
	int INFO = 0;
	int LWORK;
	double * WORK, ws_query;
	PMatrix P(N);
	
	fASSERT(M == N);
	DGETRF_F77(&M,&N,A,&M,P,&INFO);
	fASSERT(INFO >= 0); // ansonsten illegal argument
	if (INFO != 0)
		return false;

	// workspace query
	LWORK = -1;
	DGETRI_F77(&N,A,&N,P,&ws_query,&LWORK,&INFO);
	if (INFO != 0)
		return false;
	LWORK = (int)ws_query;
	WORK = new double[LWORK];

	DGETRI_F77(&N,A,&N,P,WORK,&LWORK,&INFO);
	delete[] WORK;
	return INFO == 0;
}

bool linsolve(MMatrix & A, MMatrix & XB)
{
	int N = A.rows();
	int NRHS = XB.cols();
	int INFO = 0;
	PMatrix IPIV(N);

	fASSERT(XB.rows() == size_t(N));
	DGESV_F77(&N,&NRHS,A,&N,IPIV,XB,&N,&INFO);
	fASSERT(INFO >= 0); // ansonsten illegal argument
	return INFO == 0;
}

bool linsolve(MMatrix & A, MVector & xb)
{
	int N = A.rows();
	int NRHS = 1;
	int INFO = 0;
	PMatrix IPIV(N);

	fASSERT(xb.dim() == size_t(N));
	DGESV_F77(&N,&NRHS,A,&N,IPIV,xb,&N,&INFO);
	fASSERT(INFO >= 0); // ansonsten illegal argument
	return INFO == 0;
}

bool qr(MMatrix & A, MMatrix * Q, MMatrix * R)
{
	int i,j;
	int K;
	int M = A.rows();
	int N = A.cols();
	int LDA = M;
	double * TAU;
	int INFO;
	int LWORK = -1; // workspace query
	double * WORK;
	double ws_query;

	if ((Q==0) != (R==0))
		return false;

	// falsche Dimension von Q?
	if (Q and (Q->rows() != A.rows() or Q->cols() != A.rows()))
		return false;

	// falsche Dimension von R?
	if (R and (R->rows() != A.rows() or R->cols() != A.cols()))
		return false;
	
	// Zur Rekonstruktion von Q
	TAU = new double[MIN2(M,N)];

	// Workspace query:
	DGEQRF_F77(&M,&N,A,&LDA,TAU,&ws_query,&LWORK,&INFO);

	if (INFO != 0)
	{
		delete[] TAU;
		return false;
	}

	LWORK = (int)ws_query;
	WORK = new double[LWORK];
	
	DGEQRF_F77(&M,&N,A,&LDA,TAU,WORK,&LWORK,&INFO);

	if (INFO != 0)
	{
		delete[] TAU;
		delete[] WORK;
		return false;
	}

	if (R != 0)
	{
		// R retten
		for (j=0; j<N; j++)
		{
			for (i=0; i<=MIN2(j,M-1); i++)
				R->set(i,j,A(i,j));
			for (; i<M; i++)
				R->set(i,j,0.);
		}

		/*
		// Q1 aus TAU und A berechnen und in A ablegen
		K = N;
		DORGQR_F77(&M,&N,&K,A,&LDA,TAU,WORK,&LWORK,&INFO);
		*/

		// "...where C is the identity matrix (M-by-M),
		// SIDE='L' and TRANS='N', this will compute C <- Q * I."
		char SIDE = 'L';
		char TRANS = 'N';
		int LDC = M;
		for (i=0; i<M; i++)
			for (j=0; j<M; j++)
				Q->set(i,j,i==j ? 1. : 0.);

		delete[] WORK;
		LWORK = -1; // workspace query
		K = M;
		DORMQR_F77(&SIDE,&TRANS,&M,&M,&K,A,&LDA,TAU,(*Q),&LDC,&ws_query,&LWORK,&INFO);
		if (INFO != 0)
		{
			delete[] TAU;
			return false;
		}
		LWORK = (int)ws_query;
		WORK = new double[LWORK];
		// berechne C = Q*I = Q
		DORMQR_F77(&SIDE,&TRANS,&M,&M,&K,A,&LDA,TAU,(*Q),&LDC,WORK,&LWORK,&INFO);
	}

	delete[] TAU;
	delete[] WORK;
	return INFO == 0;
}

int qr_rank(MMatrix A)
{
	int M = A.rows();
	int N = A.cols();
	int LDA = M;
	PMatrix P(N);
	int * JPVT = reinterpret_cast< int * >((unsigned int *)P);
	double * TAU;
	int INFO;
	int LWORK = -1; // workspace query
	double * WORK;
	double ws_query;
	double tol = MAX2(M,N) * 10 * MACHEPS;
	
	TAU = new double[MIN2(M,N)];
	// Workspace query:
	DGEQP3_F77(&M,&N,A,&LDA,JPVT,TAU,&ws_query,&LWORK,&INFO);
	if (INFO != 0)
	{
		delete[] TAU;
		return -1;
	}
	LWORK = (int)ws_query;
	WORK = new double[LWORK];
	
	DGEQP3_F77(&M,&N,A,&LDA,JPVT,TAU,WORK,&LWORK,&INFO);

	delete[] TAU;
	delete[] WORK;
	if (INFO != 0)
		return -1;

	for (size_t r=MIN2(M,N); r>0; r--)
	{
		bool z = true;
		for (size_t j=r-1; j<(size_t)N and z; j++)
			if (fabs(A(r-1,j)) > tol) z = false;
		if (not z) return r;
	}
	return 0;
}

bool qrp(MMatrix & A, MMatrix & Q, MMatrix & R, PMatrix & P)
{
	int i,j;
	int K;
	int M = A.rows();
	int N = A.cols();
	int LDA = M;
	int * JPVT = reinterpret_cast< int * >((unsigned int *)P);
	double * TAU;
	int INFO;
	int LWORK = -1; // workspace query
	double * WORK;
	double ws_query;

	// falsche Dimension von Q?
	if (Q.rows() != A.rows() or Q.cols() != A.rows())
		return false;

	// falsche Dimension von R?
	if (R.rows() != A.rows() or R.cols() != A.cols())
		return false;

	// falsche Dimension von P?
	if (P.dim() != A.cols())
		return false;
	
	// Zur Rekonstruktion von Q
	K = MIN2(M,N);
	TAU = new double[K];

	// Workspace query:
	DGEQP3_F77(&M,&N,A,&LDA,JPVT,TAU,&ws_query,&LWORK,&INFO);

	if (INFO != 0)
	{
		delete[] TAU;
		return false;
	}

	LWORK = (int)ws_query;
	WORK = new double[LWORK];
	
	DGEQP3_F77(&M,&N,A,&LDA,JPVT,TAU,WORK,&LWORK,&INFO);

	if (INFO != 0)
	{
		delete[] TAU;
		delete[] WORK;
		return false;
	}

	// R retten
	for (j=0; j<N; j++)
	{
		for (i=0; i<=MIN2(j,M-1); i++)
			R(i,j) = A(i,j);
		for (; i<M; i++)
			R(i,j) = 0.;
	}

	// "...where C is the identity matrix (M-by-M),
	// SIDE='L' and TRANS='N', this will compute C <- Q * I."
	char SIDE = 'L';
	char TRANS = 'N';
	int LDC = M;
	for (i=0; i<M; i++)
		for (j=0; j<M; j++)
			Q(i,j) = (i==j) ? 1. : 0.;

	delete[] WORK;
	LWORK = -1; // workspace query
	DORMQR_F77(&SIDE,&TRANS,&M,&M,&K,A,&LDA,TAU,Q,&LDC,&ws_query,&LWORK,&INFO);
	if (INFO != 0)
	{
		delete[] TAU;
		return false;
	}
	LWORK = (int)ws_query;
	WORK = new double[LWORK];
	// berechne C = Q*I = Q
	DORMQR_F77(&SIDE,&TRANS,&M,&M,&K,A,&LDA,TAU,Q,&LDC,WORK,&LWORK,&INFO);

	// Fortran-Indizierung der Permutation korrigieren 1..N -> 0..N-1
	for (i=0; i<N; i++)
		P(i) -= 1;

	delete[] TAU;
	delete[] WORK;
	return INFO == 0;
}

#if 0
int lssolve(MMatrix & A, MMatrix const & B, MMatrix & X, GVector< int > JPVT, double RCOND)
{
	int i,j;
	int M = A.rows();
	int N = A.cols();
	int NRHS = B.cols();
	int LDA = M;
	int LDB = MAX2(M,N);
	int RANK;
	double * WORK, ws_query;
	int LWORK;
	int INFO;

	if (B.rows() != A.rows() or JPVT.dim() != A.cols())
		return -1;
	if (X.rows() != A.cols() or X.cols() != B.cols())
		return -1;

	MMatrix XB(MAX2(M,N),NRHS);
	for (j=0; j<NRHS; j++)
		for (i=0; i<M; i++)
			XB(i,j) = B.get(i,j);

	// workspace query
	LWORK = -1;
	DGELSY_F77(&M,&N,&NRHS,A,&LDA,XB,&LDB,JPVT,&RCOND,&RANK,&ws_query,&LWORK,&INFO);

	if (INFO != 0)
		return -1;

	LWORK = (int)ws_query;
	WORK = new double[LWORK];

	DGELSY_F77(&M,&N,&NRHS,A,&LDA,XB,&LDB,JPVT,&RCOND,&RANK,WORK,&LWORK,&INFO);
	delete[] WORK;

	if (INFO != 0)
		return -1;

	for (j=0; j<NRHS; j++)
		for (i=0; i<N; i++)
			X(i,j) = XB(i,j);

	return RANK;
}

int lssolve(MMatrix & A, MVector const & b, MVector & x, GVector< int > JPVT, double RCOND)
{
	int i;
	int M = A.rows();
	int N = A.cols();
	int NRHS = 1;
	int LDA = M;
	int LDB = MAX2(M,N);
	int RANK;
	double * WORK, ws_query;
	int LWORK;
	int INFO;

	if (b.dim() != A.rows() or x.dim() != A.cols() or JPVT.dim() != A.cols())
		return -1;

	MVector xb(MAX2(M,N));
	for (i=0; i<M; i++)
		xb(i) = b.get(i);

	// workspace query
	LWORK = -1;
	DGELSY_F77(&M,&N,&NRHS,A,&LDA,xb,&LDB,JPVT,&RCOND,&RANK,&ws_query,&LWORK,&INFO);

	if (INFO != 0)
		return -1;

	LWORK = (int)ws_query;
	WORK = new double[LWORK];

	DGELSY_F77(&M,&N,&NRHS,A,&LDA,xb,&LDB,JPVT,&RCOND,&RANK,WORK,&LWORK,&INFO);
	delete[] WORK;

	if (INFO != 0)
		return -1;

	for (i=0; i<N; i++)
		x(i) = xb(i);

	return RANK;
}
#endif

bool lssolve(MMatrix & A, MVector const & b, MVector & x)
{
	int i;
	char TRANS = 'N';
	int M = A.rows();
	int N = A.cols();
	int NRHS = 1;
	int LDA = M;
	int LDB = MAX2(M,N);
	double * WORK, ws_query;
	int LWORK;
	int INFO;

	if (b.dim() != A.rows() or x.dim() != A.cols())
		return false;

	MVector xb(MAX2(M,N));
	for (i=0; i<M; i++)
		xb(i) = b.get(i);

	// workspace query
	LWORK = -1;
	DGELS_F77(&TRANS,&M,&N,&NRHS,A,&LDA,xb,&LDB,&ws_query,&LWORK,&INFO);

	if (INFO != 0)
		return false;

	LWORK = (int)ws_query;
	WORK = new double[LWORK];

	DGELS_F77(&TRANS,&M,&N,&NRHS,A,&LDA,xb,&LDB,WORK,&LWORK,&INFO);
	delete[] WORK;

	if (INFO != 0)
		return false;

	for (i=0; i<N; i++)
		x(i) = xb(i);

	return true;
}

bool lssolve(MMatrix & A, MMatrix const & B, MMatrix & X)
{
	int i,j;
	char TRANS = 'N';
	int M = A.rows();
	int N = A.cols();
	int NRHS = B.cols();
	int LDA = M;
	int LDB = MAX2(M,N);
	double * WORK, ws_query;
	int LWORK;
	int INFO;

	if (B.rows() != A.rows() or B.cols() != X.cols() or A.cols() != X.rows())
		return false;

	MMatrix XB(MAX2(M,N),NRHS);
	for (j=0; j<NRHS; j++)
		for (i=0; i<M; i++)
			XB(i,j) = B.get(i,j);

	// workspace query
	LWORK = -1;
	DGELS_F77(&TRANS,&M,&N,&NRHS,A,&LDA,XB,&LDB,&ws_query,&LWORK,&INFO);

	if (INFO != 0)
		return false;

	LWORK = (int)ws_query;
	WORK = new double[LWORK];

	DGELS_F77(&TRANS,&M,&N,&NRHS,A,&LDA,XB,&LDB,WORK,&LWORK,&INFO);
	delete[] WORK;

	if (INFO != 0)
		return false;

	for (j=0; j<NRHS; j++)
		for (i=0; i<N; i++)
			X(i,j) = XB(i,j);

	return true;
}

#if 0
// NICHT FERTIG!!!
bool lssolve_svd(MMatrix & A, MVector const & b, MVector & x, int & RANK, double RCOND)
{
	int i;
	int M = A.rows();
	int N = A.cols();
	int NRHS = 1;
	int LDA = M;
	int LDB = MAX2(M,N);
	double * S = 0;
	double * WORK, ws_query;
	int LWORK;
	int INFO;

	if (b.dim() != A.rows() or x.dim() != A.cols())
		return false;

	MVector xb(MAX2(M,N));
	for (i=0; i<M; i++)
		xb(i) = b.get(i);

	// workspace query
	LWORK = -1;
	DGELSS_F77(&M,&N,&NRHS,A,&LDA,xb,&LDB,S,&RCOND,&RANK,&ws_query,&LWORK,&INFO);

	if (INFO != 0)
		return false;

	LWORK = (int)ws_query;
	WORK = new double[LWORK];
	S = new double[MIN2(M,N)];

	DGELSS_F77(&M,&N,&NRHS,A,&LDA,xb,&LDB,S,&RCOND,&RANK,WORK,&LWORK,&INFO);
	delete[] WORK;

	if (INFO != 0)
		return false;

	for (i=0; i<N; i++)
		x(i) = xb(i);

	return true;
}
#endif

extern int ZGEES_SELECT(COMPLEX16) { return 1; }

bool schur(GMatrix< COMPLEX16 > & A, GVector< COMPLEX16 > & w, GMatrix< COMPLEX16 > & Z)
{
	char JOBVS = 'V';
	char SORT = 'S';
	int N = A.rows();
	int LDA = N;
	int SDIM;
	int LDVS = N;
	COMPLEX16 * WORK;
	COMPLEX16 wsquery;
	int LWORK = -1;
	double * RWORK = new double[N];
	int * BWORK = new int[N];
	int INFO;

	// Workspace Query:
	ZGEES_F77(&JOBVS,&SORT,ZGEES_SELECT,&N,A,&LDA,&SDIM,w,Z,&LDVS,&wsquery,&LWORK,RWORK,BWORK,&INFO);

	if (INFO != 0)
		return false;
	LWORK = int(wsquery.real());
	WORK = new COMPLEX16[LWORK];
	
	ZGEES_F77(&JOBVS,&SORT,ZGEES_SELECT,&N,A,&LDA,&SDIM,w,Z,&LDVS,WORK,&LWORK,RWORK,BWORK,&INFO);

	delete[] WORK;
	delete[] BWORK;
	delete[] RWORK;
	return INFO == 0;
}

bool cholesky(MMatrix & A)
{
	char UPLO = 'U';
	int N = A.rows();
	int LDA = N;
	int INFO;

	if (A.rows() != A.cols())
		return false;

	DPOTRF_F77(&UPLO, &N, A, &LDA, &INFO);
	
	return INFO == 0;
}

bool symmeig(MMatrix & A, MVector & W)
{
	char UPLO = 'U';
	int N = A.rows();
	int LDA = N;
	MVector D(N);
	MVector E(N-1);
	MVector TAU(N-1);
	int LWORK = -1; // workspace query
	double wsquery;
	double * WORK;
	int INFO;

	// workspace query
	DSYTRD_F77(&UPLO,&N,A,&LDA,D,E,TAU,&wsquery,&LWORK,&INFO);
	if (INFO != 0)
	{
		fINFO("DSYTRD workspace query returned with INFO=%i", INFO);
		return false;
	}
	LWORK = int(wsquery);
	WORK = new double[LWORK];
	DSYTRD_F77(&UPLO,&N,A,&LDA,D,E,TAU,WORK,&LWORK,&INFO);
	if (INFO != 0)
	{
		fINFO("DSYTRD returned with INFO=%i", INFO);
		delete[] WORK;
		return false;
	}

	char RANGE = 'A'; // alle Eigenwerte
	char ORDER = 'E'; // Eigenwerte sortieren
	double ABSTOL = 0.;
	int M;
	int NSPLIT;
	W = MVector(N);
	int * IBLOCK = new int[N];
	int * ISPLIT = new int[N];
	delete[] WORK; WORK = new double[4*N];
	int * IWORK = new int[3*N];
	DSTEBZ_F77(&RANGE,&ORDER,&N,0,0,0,0,&ABSTOL,D,E,&M,&NSPLIT,W,IBLOCK,ISPLIT,WORK,IWORK,&INFO);

	delete[] IBLOCK;
	delete[] ISPLIT;
	delete[] WORK;
	delete[] IWORK;
	if (INFO != 0)
		fWARNING("DSTEBZ returned INFO=%i", INFO);
	return INFO == 0;
}

} // namespace flux::la::lapack
} // namespace flux::la
} // namespace flux

