/*Copyright (c) 2014, Edgar Solomonik, all rights reserved.*/
/** \addtogroup examples 
  * @{ 
  * \defgroup fast_tensor_ctr 
  * @{ 
  * \brief A clever way to multiply symmetric tensors
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <math.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <ctf.hpp>

//gets parity of (a,b) in chi(c)
int parity(char const * a, char const * b, char const * c, int len_A, int len_B){
  char ab[len_A+len_B];
  for (int i=0; i<len_A; i++){
    ab[i] = a[i];
  }
  for (int i=0; i<len_B; i++){
    ab[len_A+i] = b[i];
  }
  /*for (int i=0; i<len_A+len_B; i++){
    printf("%c", ab[i]);
  }
  printf("\n");
  for (int i=0; i<len_A+len_B; i++){
    printf("%c", c[i]);
  }
  printf("\n");*/
  int par = 0;
  for (int i=0; i<len_A+len_B; i++){
    if (ab[i] != c[i]){
      int j;
      for (j=i+1; j<len_A+len_B; j++){
        if (ab[j] == c[i]){
          ab[j] = ab[i];
          ab[i] = c[i];
          par++;
          break;
        }
      }
      assert(j<len_A+len_B);
    }
  }
  //printf("parity = %d\n", par);
  return par;
}

//gets parity of a in chi(c)
int parity(char const * a, char const * c, int len_A, int len_C){
  if (len_A == len_C) return parity(a, NULL, c, len_A, 0);
  else {
    char b[len_C-len_A];
    int ib = 0;
    for (int i=0; i<len_C; i++){
      int j;
      for (j=0; j<len_A; j++){
        if (c[i] == a[j]) break;
      }
      if (j==len_A){
        b[ib] = c[i];
        ib++;
      }
    }
    assert(ib == len_C-len_A);
    return parity(a, b, c, len_A, len_C-len_A);
  }
}

double sign(int par){
  //printf("par = %d, %d\n",par, par%2);
  if (par%2 == 1) return -1.0;
  else return 1.0;
}


void get_rand_as_tsr(CTF_Tensor &tsr, int seed){
  int64_t * indices, size;
  double * values;
  int dim = tsr.ndim;
  if (dim == 1){
    srand48(seed);
    tsr.read_local(&size, &indices, &values);
    for (int64_t i=0; i<size; i++){
      values[i] = drand48();
    }
    tsr.write(size, indices, values);
    free(indices);
    free(values);
  } else {
    CTF_Tensor tsr_n1(dim-1, tsr.len, tsr.sym, *tsr.world);
    get_rand_as_tsr(tsr_n1, seed+1);
    CTF_Vector v(tsr.len[0], *tsr.world);
    srand48(2*seed);
    v.read_local(&size, &indices, &values);
    for (int64_t i=0; i<size; i++){
      values[i] = drand48();
    }
    v.write(size, indices, values);
    free(indices);
    free(values);
    char str[dim];
    char str_n1[dim-1];
    for (int i=0; i<dim; i++){
      str[i] = 'a'+i;
    }
    double sgn = 1.0;
    for (int i=0; i<dim; i++){
      for (int j=0; j<dim-1; j++){
        if (j>=i) str_n1[j] = str[j+1];
        else str_n1[j] =str[j];
      }
      //printf("sgn = %lf %lf\n",sgn,sign(parity(str+i, str_n1, str, 1, dim-1)));
      assert(sign(parity(str+i, str_n1, str, 1, dim-1)) == sgn);
      tsr[str] += sgn*v[str+i]*tsr_n1[str_n1];
      sgn *= -1.0;
    }
  }
}

bool check_asym(CTF_Tensor tsr){
  int dim = tsr.ndim;
  CTF_Tensor ptsr(dim, tsr.len, tsr.sym, *tsr.world);
  char str[dim];
  char pstr[dim];
  for (int i=0; i<dim; i++){
    str[i] = 'a'+i;
    pstr[i] = 'a'+i;
  }
  for (int i=0; i<dim; i++){
    for (int j=i+1; j<dim; j++){
      pstr[i] = str[j];
      pstr[j] = str[i];
      ptsr[str] += tsr[str];
      ptsr[str] += tsr[pstr];
      if (ptsr.norm2() > 1.E-6) return false;
      pstr[i] = str[i];
      pstr[j] = str[j];
    }
  }
  return true;
}

bool check_sym(CTF_Tensor tsr){
  int dim = tsr.ndim;
  CTF_Tensor ptsr(dim, tsr.len, tsr.sym, *tsr.world);
  char str[dim];
  char pstr[dim];
  for (int i=0; i<dim; i++){
    str[i] = 'a'+i;
    pstr[i] = 'a'+i;
  }
  for (int i=0; i<dim; i++){
    for (int j=i+1; j<dim; j++){
      pstr[i] = str[j];
      pstr[j] = str[i];
      ptsr[str] += tsr[str];
      ptsr[str] -= tsr[pstr];
      if (ptsr.norm2() > 1.E-6) return false;
      pstr[i] = str[i];
      pstr[j] = str[j];
    }
  }
  return true;
}

int64_t fact(int64_t n){
  int64_t f = 1;
  for (int64_t i=1; i<=n; i++){
    f*=i;
  }
  return f;
}

int64_t choose(int64_t n, int64_t k){
  return fact(n)/(fact(k)*fact(n-k));
}

int64_t chchoose(int64_t n, int64_t k){
  return fact(n+k-1)/(fact(k)*fact(n-1));
}

void chi(char const * idx,
         int          idx_len,
         int          p_len,
         int          q_len,
         int *        npair, 
         char ***     idx_p, 
         char ***     idx_q){
  int np;
  
//  printf("entering (i<%d>,j<%d>) in chi(k<%d>)\n",p_len,q_len,idx_len);

  if (p_len+q_len > idx_len){
    *npair = 0;
    return;
  }
  if (idx_len == 0 || (p_len == 0 && q_len == 0)){
    *npair = 1;
    char ** ip = (char**)malloc(sizeof(char*));
    char ** iq = (char**)malloc(sizeof(char*));
    *idx_p = ip;
    *idx_q = iq;
    return;
  }

  np  = choose(idx_len, p_len);
  np *= choose(idx_len-p_len, q_len);
  *npair = np;
//  printf("expecting %d pairs\n",np);

  char ** ip = (char**)malloc(sizeof(char*)*np);
  char ** iq = (char**)malloc(sizeof(char*)*np);
  
  *idx_p = ip;
  *idx_q = iq;

  for (int i=0; i<np; i++){
    ip[i] = (char*)malloc(sizeof(char)*p_len);
    iq[i] = (char*)malloc(sizeof(char)*q_len);
  }

  if (q_len == 0){
    char ** n1_ip;
    char ** qnull;
    int n1_len;
    chi(idx, idx_len-1, p_len-1, 0, &n1_len, &n1_ip, &qnull);

    for (int i=0; i<n1_len; i++){
      memcpy(ip[i], n1_ip[i], sizeof(char)*(p_len-1));
      ip[i][p_len-1] = idx[idx_len-1];
    }
    
    char ** n2_ip;
    int n2_len;
    chi(idx, idx_len-1, p_len, 0, &n2_len, &n2_ip, &qnull);
    assert(n2_len + n1_len == np);
    
    for (int i=0; i<n2_len; i++){
      memcpy(ip[i+n1_len], n2_ip[i], sizeof(char)*p_len);
    }
  } else if (p_len == 0){
    char ** n1_iq;
    char ** pnull;
    int n1_len;
    chi(idx, idx_len-1, 0, q_len-1, &n1_len, &pnull, &n1_iq);

    for (int i=0; i<n1_len; i++){
      memcpy(iq[i], n1_iq[i], sizeof(char)*(q_len-1));
      iq[i][q_len-1] = idx[idx_len-1];
    }
    
    char ** n2_iq;
    int n2_len;
    chi(idx, idx_len-1, 0, q_len, &n2_len, &pnull, &n2_iq);
    assert(n2_len + n1_len == np);
    
    for (int i=0; i<n2_len; i++){
      memcpy(iq[i+n1_len], n2_iq[i], sizeof(char)*q_len);
    }
  } else {
    char ** n1_ip;
    char ** n1_iq;
    int n1_len;
    chi(idx, idx_len-1, p_len-1, q_len, &n1_len, &n1_ip, &n1_iq);

    assert(n1_len<=np);
    for (int i=0; i<n1_len; i++){
      memcpy(ip[i], n1_ip[i], sizeof(char)*(p_len-1));
      ip[i][p_len-1] = idx[idx_len-1];
      memcpy(iq[i], n1_iq[i], sizeof(char)*q_len);
    }
 
    char ** n2_ip;
    char ** n2_iq;
    int n2_len;
    chi(idx, idx_len-1, p_len, q_len-1, &n2_len, &n2_ip, &n2_iq);

    for (int i=0; i<n2_len; i++){
      memcpy(ip[i+n1_len], n2_ip[i], sizeof(char)*p_len);
      memcpy(iq[i+n1_len], n2_iq[i], sizeof(char)*(q_len-1));
      iq[i+n1_len][q_len-1] = idx[idx_len-1];
    }
 
    char ** n3_ip;
    char ** n3_iq;
    int n3_len;
    chi(idx, idx_len-1, p_len, q_len, &n3_len, &n3_ip, &n3_iq);

    for (int i=0; i<n3_len; i++){
      memcpy(ip[i+n1_len+n2_len], n3_ip[i], sizeof(char)*p_len);
      memcpy(iq[i+n1_len+n2_len], n3_iq[i], sizeof(char)*q_len);
    }

    assert(n1_len+n2_len+n3_len==np);

  }
 /* printf("exiting (i<%d>,j<%d>) in chi(k<%d>) with npair=%d\n",p_len,q_len,idx_len,*npair);
  for (int i=0; i<*npair; i++){
    printf("(");
    for (int j=0; j<p_len; j++){
      printf("%c",ip[i][j]);
    }
    printf(", ");
    for (int j=0; j<q_len; j++){
      printf("%c",iq[i][j]);
    }
    printf(")\n");
  }*/
}

void chi(char const * idx,
         int          idx_len,
         int          p_len,
         int *        npair, 
         char ***     idx_p){
  char ** idx_q;

  chi(idx, idx_len, p_len, idx_len-p_len, npair, idx_p, &idx_q);
  
}

int fast_tensor_ctr(int        n,
                    int        s,
                    int        t,
                    int        v,
                    CTF_World &ctf){
  int rank, i, num_pes;
  int64_t * indices, size;
  double * values;
  
  //MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //MPI_Comm_size(MPI_COMM_WORLD, &num_pes);

  int len_A[s+v];
  int len_B[t+v];
  int len_C[s+t];

  int sym_A[s+v];
  int sym_B[t+v];
  int sym_C[s+t];
  
  char idx_A[s+v];
  char idx_B[t+v];
  char idx_C[s+t];

  for (i=0; i<s+v; i++){
    len_A[i] = n;
    sym_A[i] = NS;
    if (i<s)
      idx_A[i] = 'a'+i;
    else
      idx_A[i] = 'a'+(s+t)+(i-s);
  }
  for (i=0; i<t+v; i++){
    len_B[i] = n;
    sym_B[i] = NS;
    if (i<v)
      idx_B[i] = 'a'+(s+t)+i;
    else
      idx_B[i] = 'a'+s+(i-v);
  }
  for (i=0; i<s+t; i++){
    len_C[i] = n;
    sym_C[i] = NS;
    idx_C[i] = 'a'+i;
  }
  
  CTF_Tensor A(s+v, len_A, sym_A, ctf, "A", 1);
  CTF_Tensor B(t+v, len_B, sym_B, ctf, "B", 1);
  CTF_Tensor C(s+t, len_C, sym_C, ctf, "C", 1);
  CTF_Tensor C_int(s+t, len_C, sym_C, ctf, "C_psym", 1);
  CTF_Tensor C_ans(s+t, len_C, sym_C, ctf, "C_ans", 1);
  get_rand_as_tsr(A, 13);
  //A.print();
  assert(check_asym(A));
  if (A.ndim > 1)
    assert(!check_sym(A));
 
  get_rand_as_tsr(B, 21);
  assert(check_asym(B));
  if (B.ndim > 1)
    assert(!check_sym(B));
    
  C_int[idx_C] += A[idx_A]*B[idx_B];

  int ncperms;
  char ** idx_As;
  char ** idx_Bt;

  chi(idx_C, s+t, s, t, &ncperms, &idx_As, &idx_Bt);

  for (i=0; i<ncperms; i++){
    char idx_C_int[s+t];
    memcpy(idx_C_int, idx_As[i], sizeof(char)*s);
    memcpy(idx_C_int+s, idx_Bt[i], sizeof(char)*t);
    C_ans[idx_C] += C_int[idx_C_int];
  }
  bool is_C_sym = check_sym(C_ans);
  if (is_C_sym) printf("C_ans is symmetric\n");
  else printf("C_ans is NOT symmetric!!\n");

  int len_Z[s+v+t];
  int sym_Z[s+v+t];
  char idx_Z[s+v+t];
  for (i=0; i<s+v+t; i++){
    len_Z[i] = n;
    sym_Z[i] = NS;
    idx_Z[i] = 'a'+i;
  }

  CTF_Tensor Z_A_ops(s+v+t, len_Z, sym_Z, ctf, "Z_A", 1);
  CTF_Tensor Z_B_ops(s+v+t, len_Z, sym_Z, ctf, "Z_B", 1);
  CTF_Tensor Z_mults(s+v+t, len_Z, sym_Z, ctf, "Z", 1);

  int nAperms;
  char ** idx_Asv;

  chi(idx_Z, s+t+v, s+v, &nAperms, &idx_Asv);

  for (i=0; i<nAperms; i++){
    Z_A_ops[idx_Z] += sign(parity(idx_Asv[i], idx_Z, s+v, s+t+v))*A[idx_Asv[i]];
  }
  bool is_A_asym = check_asym(Z_A_ops);
  if (is_A_asym) printf("Z_A_ops is antisymmetric\n");
  else printf("Z_A_ops is NOT antisymmetric!!\n");
  
  int nBperms;
  char ** idx_Btv;
  
  chi(idx_Z, s+t+v, t+v, &nBperms, &idx_Btv);

  for (i=0; i<nBperms; i++){
    Z_B_ops[idx_Z] += sign(parity(idx_Btv[i], idx_Z, t+v, s+t+v))*B[idx_Btv[i]];
  }
  bool is_B_asym = check_asym(Z_B_ops);
  if (is_B_asym) printf("Z_B_ops is antisymmetric\n");
  else printf("Z_B_ops is NOT antisymmetric!!\n");
  

  Z_mults[idx_Z] = Z_A_ops[idx_Z]*Z_B_ops[idx_Z];

  bool is_Z_sym = check_sym(Z_mults);
  if (is_Z_sym) printf("Z_mults is symmetric\n");
  else printf("Z_mults is NOT symmetric!!\n");

  memcpy(idx_Z,idx_C,(s+t)*sizeof(char));
  for (i=s+t; i<s+t+v; i++){
    idx_Z[i] = idx_Z[s+t-1]+(i-s-t+1);
  }

  for (int j=0; j<2; j++)
    printf("idx_Z[%d]=%c,idx_C[%d]=%c\n",j,idx_Z[j],j,idx_C[j]);
  //C[idx_C]+=sign(s*t+s*v+t*v)*Z_mults[idx_Z];
  C[idx_C]+=sign(s*t+s*v)*Z_mults[idx_Z];

  CTF_Tensor V(s+t, len_C, sym_C, ctf, "V");
  for (int r=0; r<v; r++){
    for (int p=std::max(v-t-r,0); p<=v-r; p++){
      for (int q=std::max(v-s-r,0); q<=v-p-r; q++){
        double prefact = (double)(choose(v,r)*choose(v-r,p)*choose(v-p-r,q)*pow(n,v-p-q-r));
//        double sgn_V = sign(s*t+(p+q)*v+p*q);
        double sgn_V = sign(s*t+v*(t+p+r)+p*q+(q+r)*(t+1));
//+(p+q)*v+p*q);
        prefact*= sgn_V;
  
        char idx_kr[r];
        for (i=0; i<r; i++){
          idx_kr[i] = 'a'+s+t+i;
        }
        char idx_kp[p];
        for (i=0; i<p; i++){
          idx_kp[i] = 'a'+s+t+r+i;
        }
        char idx_kq[q];
        for (i=0; i<q; i++){
          idx_kq[i] = 'a'+s+t+r+p+i;
        }

        CTF_Tensor V_A_ops(s+t+r, len_Z, sym_Z, ctf, "V_A_ops");
        char idx_VA[s+t+r];
        memcpy(idx_VA,idx_C,(s+t)*sizeof(char));
        memcpy(idx_VA+s+t,idx_kr,r*sizeof(char));
        //printf("r=%d,p=%d,q=%d\n",r,p,q);
        int nvAperms;
        char ** idx_VAsvpr;
        chi(idx_C, s+t, s+v-p-r, &nvAperms, &idx_VAsvpr);
        for (i=0; i<nvAperms; i++){
          char idx_VAA[s+v];
          memcpy(idx_VAA, idx_VAsvpr[i], (s+v-p-r)*sizeof(char));
          memcpy(idx_VAA+s+v-p-r, idx_kr, r*sizeof(char));
          memcpy(idx_VAA+s+v-p, idx_kp, p*sizeof(char));
          double sgn_VA = sign(parity(idx_VAsvpr[i], idx_C, s+v-p-r, s+t));
          V_A_ops[idx_VA] += sgn_VA*A[idx_VAA];
        }

        CTF_Tensor V_B_ops(s+t+r, len_Z, sym_Z, ctf, "V_B_ops");
        char idx_VB[s+t+r];
        memcpy(idx_VB,idx_C,(s+t)*sizeof(char));
        memcpy(idx_VB+s+t,idx_kr,r*sizeof(char));


        int nvBperms;
        char ** idx_VBtvqr;
        chi(idx_C, s+t, t+v-q-r, &nvBperms, &idx_VBtvqr);
        for (i=0; i<nvBperms; i++){
          char idx_VBB[t+v];
          /*memcpy(idx_VBB, idx_VBtvqr[i], (t+v-q-r)*sizeof(char));
          memcpy(idx_VBB+t+v-q-r, idx_kr, r*sizeof(char));
          memcpy(idx_VBB+t+v-q, idx_kq, q*sizeof(char));*/
          memcpy(idx_VBB, idx_kr, r*sizeof(char));
          memcpy(idx_VBB+r, idx_kq, q*sizeof(char));
          memcpy(idx_VBB+r+q, idx_VBtvqr[i], (t+v-q-r)*sizeof(char));
          /*for (i=0; i<t+v; i++){
            printf("index %d of B is %c\n",i, idx_VBB[i]);
          }
          for (i=0; i<s+t+r; i++){
            printf("index %d of V_B is %c\n",i, idx_VB[i]);
          }*/
          double sgn_VB = sign(parity(idx_VBtvqr[i], idx_C, t+v-q-r, s+t));
          V_B_ops[idx_VB] += sgn_VB*B[idx_VBB];
        }

        V[idx_C] += prefact*V_A_ops[idx_VA]*V_B_ops[idx_VB];
      }
    }
  }
  CTF_Tensor W(s+t, len_C, sym_C, ctf, "W");
  for (int r=1; r<=std::min(s,t); r++){
    char idx_kr[r];
    for (int i=0; i<r; i++){
      idx_kr[i] = 'a'+s+t+i;
    }
    char idx_kv[v];
    for (int i=0; i<v; i++){
      idx_kv[i] = 'a'+s+t+r+i;
    }
    CTF_Tensor U(s+t-r, len_C, sym_C, ctf, "U");
    char idx_U[s+t-r];
    char idx_UA[s+v];
    char idx_UB[t+v];
    memcpy(idx_U, idx_kr, sizeof(char)*r);
    memcpy(idx_UA, idx_kr, sizeof(char)*r);
    memcpy(idx_UB+t+v-r, idx_kr, sizeof(char)*r);
    int npermU;
    char ** idxj, ** idxl;
    chi(idx_C, s+t-2*r, s-r, t-r, &npermU, &idxj, &idxl);
    memcpy(idx_U+r,idx_C,s+t-2*r);
    for (int i=0; i<npermU; i++){
      memcpy(idx_UA+r,idxj[i],s-r);
      memcpy(idx_UB+v,idxl[i],t-r);
      memcpy(idx_UA+s, idx_kv, sizeof(char)*v);
      //memcpy(idx_UB+t, idx_kv, sizeof(char)*v);
      memcpy(idx_UB, idx_kv, sizeof(char)*v);
//      double sgnU = sign(parity(idxj[i], idxl[i], idx_C, s-r, t-r));
      for (int j=0; j<1; j++)
        printf("idx_U[%d]=%c,idx_UA[%d]=%c,idx_UB[%d]=%c\n",j,idx_U[j],j,idx_UA[j],j,idx_UB[j]);
      //for (int j=1; j<2; j++)
        //printf("idx_UA[%d]=%c,idx_UB[%d]=%c\n",j,idx_UA[j],j,idx_UB[j]);
      U[idx_U] += A[idx_UA]*B[idx_UB];
    }
    int npermW1;
    char ** idxh1;
    chi(idx_C, s+t, s+t-r, &npermW1,&idxh1);
    for (int j=0; j<npermW1; j++){
//      double sgn1 = sign(parity(idxh1[j], idx_C, s+t-r, s+t));
      int npermW;
      char ** idxh, ** idxr;
      chi(idxh1[j], s+t-r, r, s+t-2*r, &npermW, &idxr, &idxh);
      for (int i=0; i<npermW; i++){
        memcpy(idx_U,idxr[i],r);
        memcpy(idx_U+r,idxh[i],s+t-2*r);
        //W[idx_C] += sgn1*sign(parity(idxr[i], idxh[i],idxh1[j], r,s+t-2*r))*U[idx_U];
        W[idx_C] -= U[idx_U];
      }
    }
  }
  assert(check_sym(C));
  assert(check_sym(V));
  assert(check_sym(W));
  
  C[idx_C] -= V[idx_C];
  C[idx_C] -= W[idx_C];

  C[idx_C] -= C_ans[idx_C];

  double nrm = C.norm2();

  printf("error 2-norm is %.4E\n",nrm);

  int pass = (nrm <=1.E-3);
  
//  if (rank == 0){
    if (pass) printf("{ fast symmetric tensor contraction algorithm test } passed\n");
    else      printf("{ fast symmetric tensor contraction algorithm test } failed\n");
//  } 
  return pass;
}

#ifndef TEST_SUITE
char* getCmdOption(char ** begin,
                   char ** end,
                   const   std::string & option){
  char ** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end){
    return *itr;
  }
  return 0;
}


int main(int argc, char ** argv){
  int rank, np, n, s,t,v;
  int const in_num = argc;
  char ** input_str = argv;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  if (getCmdOption(input_str, input_str+in_num, "-n")){
    n = atoi(getCmdOption(input_str, input_str+in_num, "-n"));
    if (n < 0) n = 4;
  } else n = 4;

  if (getCmdOption(input_str, input_str+in_num, "-s")){
    s = atoi(getCmdOption(input_str, input_str+in_num, "-s"));
    if (s < 0) s = 1;
  } else s = 1;

  if (getCmdOption(input_str, input_str+in_num, "-t")){
    t = atoi(getCmdOption(input_str, input_str+in_num, "-t"));
    if (t < 0) t = 1;
  } else t = 1;

  if (getCmdOption(input_str, input_str+in_num, "-v")){
    v = atoi(getCmdOption(input_str, input_str+in_num, "-v"));
    if (v < 0) v = 1;
  } else v = 1;

  {
    CTF_World dw(MPI_COMM_WORLD, argc, argv);
    if (rank == 0){
      printf("Contracting symmetric A of order %d with B of order %d into C of order %d, all with dimension %d\n",s+v,t+v,s+t,n);
    }
    int pass = fast_tensor_ctr(n, s, t, v, dw);
    assert(pass);
  }
  
  MPI_Finalize();
  return 0;
}
/**
 * @} 
 * @}
 */

#endif

