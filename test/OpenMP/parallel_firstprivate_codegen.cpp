// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple %itanium_abi_triple -emit-llvm %s -o - | FileCheck %s
// RUN: %clang_cc1 -fopenmp -x c++ -std=c++11 -triple %itanium_abi_triple -emit-pch -o %t %s
// RUN: %clang_cc1 -fopenmp -x c++ -triple %itanium_abi_triple -std=c++11 -include-pch %t -verify %s -emit-llvm -o - | FileCheck %s
// RUN: %clang_cc1 -verify -fopenmp -x c++ -std=c++11 -DLAMBDA -triple %itanium_abi_triple -emit-llvm %s -o - | FileCheck -check-prefix=LAMBDA %s
// RUN: %clang_cc1 -verify -fopenmp -x c++ -fblocks -DBLOCKS -triple %itanium_abi_triple -emit-llvm %s -o - | FileCheck -check-prefix=BLOCKS %s
// RUN: %clang_cc1 -verify -fopenmp -x c++ -std=c++11 -DARRAY -triple x86_64-apple-darwin10 -emit-llvm %s -o - | FileCheck -check-prefix=ARRAY %s
// expected-no-diagnostics
#ifndef ARRAY
#ifndef HEADER
#define HEADER

struct St {
  int a, b;
  St() : a(0), b(0) {}
  St(const St &st) : a(st.a + st.b), b(0) {}
  ~St() {}
};

volatile int g __attribute__((aligned(128))) = 1212;

struct SS {
  int a;
  int b : 4;
  int &c;
  int e[4];
  SS(int &d) : a(0), b(0), c(d) {
#pragma omp parallel firstprivate(a, b, c, e)
#ifdef LAMBDA
    [&]() {
      ++this->a, --b, (this)->c /= 1;
#pragma omp parallel firstprivate(a, b, c)
      ++(this)->a, --b, this->c /= 1;
    }();
#elif defined(BLOCKS)
    ^{
      ++a;
      --this->b;
      (this)->c /= 1;
#pragma omp parallel firstprivate(a, b, c)
      ++(this)->a, --b, this->c /= 1;
    }();
#else
    ++this->a, --b, c /= 1, e[2] = 1111;
#endif
  }
};

template<typename T>
struct SST {
  T a;
  SST() : a(T()) {
#pragma omp parallel firstprivate(a)
#ifdef LAMBDA
    [&]() {
      [&]() {
        ++this->a;
#pragma omp parallel firstprivate(a)
        ++(this)->a;
      }();
    }();
#elif defined(BLOCKS)
    ^{
      ^{
        ++a;
#pragma omp parallel firstprivate(a)
        ++(this)->a;
      }();
    }();
#else
    ++(this)->a;
#endif
  }
};

template <class T>
struct S {
  T f;
  S(T a) : f(a + g) {}
  S() : f(g) {}
  S(const S &s, St t = St()) : f(s.f + t.a) {}
  operator T() { return T(); }
  ~S() {}
};

// CHECK: [[SS_TY:%.+]] = type { i{{[0-9]+}}, i8
// LAMBDA: [[SS_TY:%.+]] = type { i{{[0-9]+}}, i8
// BLOCKS: [[SS_TY:%.+]] = type { i{{[0-9]+}}, i8
// CHECK-DAG: [[S_FLOAT_TY:%.+]] = type { float }
// CHECK-DAG: [[S_INT_TY:%.+]] = type { i{{[0-9]+}} }
// CHECK-DAG: [[ST_TY:%.+]] = type { i{{[0-9]+}}, i{{[0-9]+}} }

template <typename T>
T tmain() {
  S<T> test;
  SST<T> sst;
  T t_var __attribute__((aligned(128))) = T();
  T vec[] __attribute__((aligned(128))) = {1, 2};
  S<T> s_arr[] __attribute__((aligned(128))) = {1, 2};
  S<T> var __attribute__((aligned(128))) (3);
#pragma omp parallel firstprivate(t_var, vec, s_arr, var)
  {
    vec[0] = t_var;
    s_arr[0] = var;
  }
#pragma omp parallel firstprivate(t_var)
  {}
  return T();
}

int main() {
  static int sivar;
  SS ss(sivar);
#ifdef LAMBDA
  // LAMBDA: [[G:@.+]] = global i{{[0-9]+}} 1212,
  // LAMBDA-LABEL: @main
  // LAMBDA: alloca [[SS_TY]],
  // LAMBDA: alloca [[CAP_TY:%.+]],
  // LAMBDA: call{{.*}} void [[OUTER_LAMBDA:@[^(]+]]([[CAP_TY]]*
  [&]() {
  // LAMBDA: define{{.*}} internal{{.*}} void [[OUTER_LAMBDA]](
  // LAMBDA: call {{.*}}void {{.+}} @__kmpc_fork_call({{.+}}, i32 2, {{.+}}* [[OMP_REGION:@.+]] to {{.+}}, i32* [[G]], {{.+}})
#pragma omp parallel firstprivate(g, sivar)
  {
    // LAMBDA: define {{.+}} @{{.+}}([[SS_TY]]*
    // LAMBDA: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 0
    // LAMBDA: store i{{[0-9]+}} 0, i{{[0-9]+}}* %
    // LAMBDA: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 1
    // LAMBDA: store i8
    // LAMBDA: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 2
    // LAMBDA: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 0
    // LAMBDA: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 1
    // LAMBDA: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 2
    // LAMBDA: call void (%{{.+}}*, i{{[0-9]+}}, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)*, ...) @__kmpc_fork_call(%{{.+}}* @{{.+}}, i{{[0-9]+}} 5, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)* bitcast (void (i{{[0-9]+}}*, i{{[0-9]+}}*, [[SS_TY]]*, i32, i32, i32, [4 x i{{[0-9]+}}]*)* [[SS_MICROTASK:@.+]] to void
    // LAMBDA: ret

    // LAMBDA: define internal void [[SS_MICROTASK]](i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}}, [[SS_TY]]* %{{.+}}, i32 {{.+}}, i32 {{.+}}, i32 {{.+}}, [4 x i{{[0-9]+}}]* {{.+}})
    // LAMBDA-NOT: getelementptr {{.*}}[[SS_TY]], [[SS_TY]]* %
    // LAMBDA: call{{.*}} void
    // LAMBDA: ret void

    // LAMBDA: define internal void @{{.+}}(i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}}, [[SS_TY]]* %{{.+}}, i32 {{.+}}, i32 {{.+}}, i32 {{.+}})
    // LAMBDA: [[A_PRIV:%.+]] = alloca i{{[0-9]+}},
    // LAMBDA: [[B_PRIV:%.+]] = alloca i{{[0-9]+}},
    // LAMBDA: [[C_PRIV:%.+]] = alloca i{{[0-9]+}},
    // LAMBDA: store i{{[0-9]+}}* [[A_PRIV]], i{{[0-9]+}}** [[REFA:%.+]],
    // LAMBDA: store i{{[0-9]+}}* [[C_PRIV]], i{{[0-9]+}}** [[REFC:%.+]],
    // LAMBDA-NEXT: [[A_PRIV:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[REFA]],
    // LAMBDA-NEXT: [[A_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[A_PRIV]],
    // LAMBDA-NEXT: [[INC:%.+]] = add nsw i{{[0-9]+}} [[A_VAL]], 1
    // LAMBDA-NEXT: store i{{[0-9]+}} [[INC]], i{{[0-9]+}}* [[A_PRIV]],
    // LAMBDA-NEXT: [[B_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[B_PRIV]],
    // LAMBDA-NEXT: [[DEC:%.+]] = add nsw i{{[0-9]+}} [[B_VAL]], -1
    // LAMBDA-NEXT: store i{{[0-9]+}} [[DEC]], i{{[0-9]+}}* [[B_PRIV]],
    // LAMBDA-NEXT: [[C_PRIV:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[REFC]],
    // LAMBDA-NEXT: [[C_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[C_PRIV]],
    // LAMBDA-NEXT: [[DIV:%.+]] = sdiv i{{[0-9]+}} [[C_VAL]], 1
    // LAMBDA-NEXT: store i{{[0-9]+}} [[DIV]], i{{[0-9]+}}* [[C_PRIV]],
    // LAMBDA-NEXT: ret void

    // LAMBDA: define{{.*}} internal{{.*}} void [[OMP_REGION]](i32* noalias %{{.+}}, i32* noalias %{{.+}}, i32* dereferenceable(4) %{{.+}}, i32 {{.*}}%{{.+}})
    // LAMBDA: [[SIVAR_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}},
    // LAMBDA: [[G_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}}, align 128
    // LAMBDA: [[G_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[G_REF_ADDR:%.+]]
    // LAMBDA: [[G_VAL:%.+]] = load volatile i{{[0-9]+}}, i{{[0-9]+}}* [[G_REF]], align 128
    // LAMBDA: store i{{[0-9]+}} [[G_VAL]], i{{[0-9]+}}* [[G_PRIVATE_ADDR]], align 128
    // LAMBDA-NOT: call {{.*}}void @__kmpc_barrier(
    g = 1;
    sivar = 2;
    // LAMBDA: store i{{[0-9]+}} 1, i{{[0-9]+}}* [[G_PRIVATE_ADDR]],
    // LAMBDA: store i{{[0-9]+}} 2, i{{[0-9]+}}* [[SIVAR_PRIVATE_ADDR]],
    // LAMBDA: [[G_PRIVATE_ADDR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG:%.+]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
    // LAMBDA: store i{{[0-9]+}}* [[G_PRIVATE_ADDR]], i{{[0-9]+}}** [[G_PRIVATE_ADDR_REF]]
    // LAMBDA: [[SIVAR_PRIVATE_ADDR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG:%.+]], i{{[0-9]+}} 0, i{{[0-9]+}} 1
    // LAMBDA: store i{{[0-9]+}}* [[SIVAR_PRIVATE_ADDR]], i{{[0-9]+}}** [[SIVAR_PRIVATE_ADDR_REF]]
    // LAMBDA: call{{.*}} void [[INNER_LAMBDA:@.+]](%{{.+}}* [[ARG]])
    [&]() {
      // LAMBDA: define {{.+}} void [[INNER_LAMBDA]](%{{.+}}* [[ARG_PTR:%.+]])
      // LAMBDA: store %{{.+}}* [[ARG_PTR]], %{{.+}}** [[ARG_PTR_REF:%.+]],
      g = 2;
      sivar = 4;
      // LAMBDA: [[ARG_PTR:%.+]] = load %{{.+}}*, %{{.+}}** [[ARG_PTR_REF]]
      // LAMBDA: [[G_PTR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG_PTR]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
      // LAMBDA: [[G_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[G_PTR_REF]]
      // LAMBDA: [[SIVAR_PTR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG_PTR]], i{{[0-9]+}} 0, i{{[0-9]+}} 1
      // LAMBDA: [[SIVAR_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[SIVAR_PTR_REF]]
      // LAMBDA: store i{{[0-9]+}} 4, i{{[0-9]+}}* [[SIVAR_REF]]
    }();
  }
  }();
  return 0;
#elif defined(BLOCKS)
  // BLOCKS: [[G:@.+]] = global i{{[0-9]+}} 1212,
  // BLOCKS-LABEL: @main
  // BLOCKS: call
  // BLOCKS: call {{.*}}void {{%.+}}(i8
  ^{
  // BLOCKS: define{{.*}} internal{{.*}} void {{.+}}(i8*
  // BLOCKS: call {{.*}}void {{.+}} @__kmpc_fork_call({{.+}}, i32 2, {{.+}}* [[OMP_REGION:@.+]] to {{.+}}, i32* [[G]], {{.+}})
#pragma omp parallel firstprivate(g, sivar)
  {
    // BLOCKS: define{{.*}} internal{{.*}} void [[OMP_REGION]](i32* noalias %{{.+}}, i32* noalias %{{.+}}, i32* dereferenceable(4) %{{.+}}, i32 {{.*}}%{{.+}})
    // BLOCKS: [[SIVAR_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}},
    // BLOCKS: [[G_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}}, align 128
    // BLOCKS: [[G_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[G_REF_ADDR:%.+]]
    // BLOCKS: [[G_VAL:%.+]] = load volatile i{{[0-9]+}}, i{{[0-9]+}}* [[G_REF]], align 128
    // BLOCKS: store i{{[0-9]+}} [[G_VAL]], i{{[0-9]+}}* [[G_PRIVATE_ADDR]], align 128
    // BLOCKS-NOT: call {{.*}}void @__kmpc_barrier(
    g = 1;
    sivar = 2;
    // BLOCKS: store i{{[0-9]+}} 1, i{{[0-9]+}}* [[G_PRIVATE_ADDR]],
    // BLOCKS: store i{{[0-9]+}} 2, i{{[0-9]+}}* [[SIVAR_PRIVATE_ADDR]],
    // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
    // BLOCKS: i{{[0-9]+}}* [[G_PRIVATE_ADDR]]
    // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
    // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
    // BLOCKS: i{{[0-9]+}}* [[SIVAR_PRIVATE_ADDR]]
    // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
    // BLOCKS: call {{.*}}void {{%.+}}(i8
    ^{
      // BLOCKS: define {{.+}} void {{@.+}}(i8*
      g = 2;
      sivar = 4;
      // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
      // BLOCKS: store i{{[0-9]+}} 2, i{{[0-9]+}}*
      // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
      // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
      // BLOCKS: store i{{[0-9]+}} 4, i{{[0-9]+}}*
      // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
      // BLOCKS: ret
    }();
  }
  }();
  return 0;
// BLOCKS: define {{.+}} @{{.+}}([[SS_TY]]*
// BLOCKS: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 0
// BLOCKS: store i{{[0-9]+}} 0, i{{[0-9]+}}* %
// BLOCKS: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 1
// BLOCKS: store i8
// BLOCKS: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 2
// BLOCKS: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 0
// BLOCKS: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 1
// BLOCKS: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 2
// BLOCKS: call void (%{{.+}}*, i{{[0-9]+}}, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)*, ...) @__kmpc_fork_call(%{{.+}}* @{{.+}}, i{{[0-9]+}} 5, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)* bitcast (void (i{{[0-9]+}}*, i{{[0-9]+}}*, [[SS_TY]]*, i32, i32, i32, [4 x i{{[0-9]+}}]*)* [[SS_MICROTASK:@.+]] to void
// BLOCKS: ret

// BLOCKS: define internal void [[SS_MICROTASK]](i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}}, [[SS_TY]]* %{{.+}}, i32 {{.+}}, i32 {{.+}}, i32 {{.+}}, [4 x i{{[0-9]+}}]* {{.+}})
// BLOCKS-NOT: getelementptr {{.*}}[[SS_TY]], [[SS_TY]]* %
// BLOCKS: call{{.*}} void
// BLOCKS: ret void

// BLOCKS: define internal void @{{.+}}(i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}}, [[SS_TY]]* %{{.+}}, i32 {{.+}}, i32 {{.+}}, i32 {{.+}})
// BLOCKS: [[A_PRIV:%.+]] = alloca i{{[0-9]+}},
// BLOCKS: [[B_PRIV:%.+]] = alloca i{{[0-9]+}},
// BLOCKS: [[C_PRIV:%.+]] = alloca i{{[0-9]+}},
// BLOCKS: store i{{[0-9]+}}* [[A_PRIV]], i{{[0-9]+}}** [[REFA:%.+]],
// BLOCKS: store i{{[0-9]+}}* [[C_PRIV]], i{{[0-9]+}}** [[REFC:%.+]],
// BLOCKS-NEXT: [[A_PRIV:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[REFA]],
// BLOCKS-NEXT: [[A_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[A_PRIV]],
// BLOCKS-NEXT: [[INC:%.+]] = add nsw i{{[0-9]+}} [[A_VAL]], 1
// BLOCKS-NEXT: store i{{[0-9]+}} [[INC]], i{{[0-9]+}}* [[A_PRIV]],
// BLOCKS-NEXT: [[B_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[B_PRIV]],
// BLOCKS-NEXT: [[DEC:%.+]] = add nsw i{{[0-9]+}} [[B_VAL]], -1
// BLOCKS-NEXT: store i{{[0-9]+}} [[DEC]], i{{[0-9]+}}* [[B_PRIV]],
// BLOCKS-NEXT: [[C_PRIV:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[REFC]],
// BLOCKS-NEXT: [[C_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[C_PRIV]],
// BLOCKS-NEXT: [[DIV:%.+]] = sdiv i{{[0-9]+}} [[C_VAL]], 1
// BLOCKS-NEXT: store i{{[0-9]+}} [[DIV]], i{{[0-9]+}}* [[C_PRIV]],
// BLOCKS-NEXT: ret void
#else
  S<float> test;
  int t_var = 0;
  int vec[] = {1, 2};
  S<float> s_arr[] = {1, 2};
  S<float> var(3);
#pragma omp parallel firstprivate(t_var, vec, s_arr, var, sivar)
  {
    vec[0] = t_var;
    s_arr[0] = var;
    sivar = 2;
  }
#pragma omp parallel firstprivate(t_var)
  {}
  return tmain<int>();
#endif
}

// CHECK: define {{.*}}i{{[0-9]+}} @main()
// CHECK: [[TEST:%.+]] = alloca [[S_FLOAT_TY]],
// CHECK: call {{.*}} [[S_FLOAT_TY_DEF_CONSTR:@.+]]([[S_FLOAT_TY]]* [[TEST]])
// CHECK: call {{.*}}void (%{{.+}}*, i{{[0-9]+}}, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)*, ...) @__kmpc_fork_call(%{{.+}}* @{{.+}}, i{{[0-9]+}} 5, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)* bitcast (void (i{{[0-9]+}}*, i{{[0-9]+}}*, [2 x i32]*, i32, [2 x [[S_FLOAT_TY]]]*, [[S_FLOAT_TY]]*, i{{[0-9]+}})* [[MAIN_MICROTASK:@.+]] to void
// CHECK: = call {{.*}}i{{.+}} [[TMAIN_INT:@.+]]()
// CHECK: call {{.*}} [[S_FLOAT_TY_DESTR:@.+]]([[S_FLOAT_TY]]*
// CHECK: ret
//
// CHECK: define internal {{.*}}void [[MAIN_MICROTASK]](i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}}, [2 x i32]* dereferenceable(8) %{{.+}}, i32 {{.*}}%{{.+}}, [2 x [[S_FLOAT_TY]]]* dereferenceable(8) %{{.+}}, [[S_FLOAT_TY]]* dereferenceable(4) %{{.+}}, i32 {{.*}}[[SIVAR:%.+]])
// CHECK: [[T_VAR_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[SIVAR7_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[VEC_PRIV:%.+]] = alloca [2 x i{{[0-9]+}}],
// CHECK: [[S_ARR_PRIV:%.+]] = alloca [2 x [[S_FLOAT_TY]]],
// CHECK: [[VAR_PRIV:%.+]] = alloca [[S_FLOAT_TY]],
// CHECK: store i{{[0-9]+}}* [[GTID_ADDR]], i{{[0-9]+}}** [[GTID_ADDR_ADDR:%.+]],

// CHECK: [[VEC_REF:%.+]] = load [2 x i{{[0-9]+}}]*, [2 x i{{[0-9]+}}]** %
// CHECK-NOT: load i{{[0-9]+}}*, i{{[0-9]+}}** %
// CHECK: [[S_ARR_REF:%.+]] = load [2 x [[S_FLOAT_TY]]]*, [2 x [[S_FLOAT_TY]]]** %
// CHECK: [[VAR_REF:%.+]] = load [[S_FLOAT_TY]]*, [[S_FLOAT_TY]]** %
// CHECK-NOT: load i{{[0-9]+}}*, i{{[0-9]+}}** %
// CHECK: [[VEC_DEST:%.+]] = bitcast [2 x i{{[0-9]+}}]* [[VEC_PRIV]] to i8*
// CHECK: [[VEC_SRC:%.+]] = bitcast [2 x i{{[0-9]+}}]* [[VEC_REF]] to i8*
// CHECK: call void @llvm.memcpy.{{.+}}(i8* [[VEC_DEST]], i8* [[VEC_SRC]],
// CHECK: [[S_ARR_PRIV_BEGIN:%.+]] = getelementptr inbounds [2 x [[S_FLOAT_TY]]], [2 x [[S_FLOAT_TY]]]* [[S_ARR_PRIV]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
// CHECK: [[S_ARR_BEGIN:%.+]] = bitcast [2 x [[S_FLOAT_TY]]]* [[S_ARR_REF]] to [[S_FLOAT_TY]]*
// CHECK: [[S_ARR_PRIV_END:%.+]] = getelementptr [[S_FLOAT_TY]], [[S_FLOAT_TY]]* [[S_ARR_PRIV_BEGIN]], i{{[0-9]+}} 2
// CHECK: [[IS_EMPTY:%.+]] = icmp eq [[S_FLOAT_TY]]* [[S_ARR_PRIV_BEGIN]], [[S_ARR_PRIV_END]]
// CHECK: br i1 [[IS_EMPTY]], label %[[S_ARR_BODY_DONE:.+]], label %[[S_ARR_BODY:.+]]
// CHECK: [[S_ARR_BODY]]
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR:@.+]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_FLOAT_TY_COPY_CONSTR:@.+]]([[S_FLOAT_TY]]* {{.+}}, [[S_FLOAT_TY]]* {{.+}}, [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR:@.+]]([[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: br i1 {{.+}}, label %{{.+}}, label %[[S_ARR_BODY]]
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_FLOAT_TY_COPY_CONSTR]]([[S_FLOAT_TY]]* [[VAR_PRIV]], [[S_FLOAT_TY]]* {{.*}} [[VAR_REF]], [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR]]([[ST_TY]]* [[ST_TY_TEMP]])

// CHECK: store i{{[0-9]+}} 2, i{{[0-9]+}}* [[SIVAR7_PRIV]],

// CHECK-DAG: call {{.*}} [[S_FLOAT_TY_DESTR]]([[S_FLOAT_TY]]* [[VAR_PRIV]])
// CHECK-DAG: call {{.*}} [[S_FLOAT_TY_DESTR]]([[S_FLOAT_TY]]*
// CHECK: ret void
// CHECK: define {{.*}} i{{[0-9]+}} [[TMAIN_INT]]()
// CHECK: [[TEST:%.+]] = alloca [[S_INT_TY]],
// CHECK: call {{.*}} [[S_INT_TY_DEF_CONSTR:@.+]]([[S_INT_TY]]* [[TEST]])
// CHECK: call {{.*}}void (%{{.+}}*, i{{[0-9]+}}, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)*, ...) @__kmpc_fork_call(%{{.+}}* @{{.+}}, i{{[0-9]+}} 4, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)* bitcast (void (i{{[0-9]+}}*, i{{[0-9]+}}*, [2 x i32]*, i32*, [2 x [[S_INT_TY]]]*, [[S_INT_TY]]*)* [[TMAIN_MICROTASK:@.+]] to void
// CHECK: call {{.*}} [[S_INT_TY_DESTR:@.+]]([[S_INT_TY]]*
// CHECK: ret
//
// CHECK: define {{.+}} @{{.+}}([[SS_TY]]*
// CHECK: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 0
// CHECK: store i{{[0-9]+}} 0, i{{[0-9]+}}* %
// CHECK: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 1
// CHECK: store i8
// CHECK: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 2
// CHECK: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 0
// CHECK: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 1
// CHECK: getelementptr inbounds [[SS_TY]], [[SS_TY]]* %{{.+}}, i32 0, i32 2
// CHECK: call void (%{{.+}}*, i{{[0-9]+}}, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)*, ...) @__kmpc_fork_call(%{{.+}}* @{{.+}}, i{{[0-9]+}} 5, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)* bitcast (void (i{{[0-9]+}}*, i{{[0-9]+}}*, [[SS_TY]]*, i32, i32, i32, [4 x i32]*)* [[SS_MICROTASK:@.+]] to void
// CHECK: ret

// CHECK: define internal void [[SS_MICROTASK]](i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}}, [[SS_TY]]* %{{.+}}, i32 {{.+}}, i32 {{.+}}, i32 {{.+}}, [4 x i{{[0-9]+}}]* {{.+}})
// CHECK: [[A_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[B_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[C_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[E_PRIV:%.+]] = alloca [4 x i{{[0-9]+}}],
// CHECK: store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[A_PRIV]]
// CHECK: store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[B_PRIV]]
// CHECK: store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[C_PRIV]]
// CHECK: store i{{[0-9]+}}* [[A_PRIV]], i{{[0-9]+}}** [[REFA:%.+]],
// CHECK: store i{{[0-9]+}}* [[C_PRIV]], i{{[0-9]+}}** [[REFC:%.+]],
// CHECK: bitcast [4 x i{{[0-9]+}}]* [[E_PRIV]] to i8*
// CHECK: bitcast [4 x i{{[0-9]+}}]* %{{.+}} to i8*
// CHECK: call void @llvm.memcpy
// CHECK: store [4 x i{{[0-9]+}}]* [[E_PRIV]], [4 x i{{[0-9]+}}]** [[REFE:%.+]],
// CHECK-NEXT: [[A_PRIV:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[REFA]],
// CHECK-NEXT: [[A_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[A_PRIV]],
// CHECK-NEXT: [[INC:%.+]] = add nsw i{{[0-9]+}} [[A_VAL]], 1
// CHECK-NEXT: store i{{[0-9]+}} [[INC]], i{{[0-9]+}}* [[A_PRIV]],
// CHECK-NEXT: [[B_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[B_PRIV]],
// CHECK-NEXT: [[DEC:%.+]] = add nsw i{{[0-9]+}} [[B_VAL]], -1
// CHECK-NEXT: store i{{[0-9]+}} [[DEC]], i{{[0-9]+}}* [[B_PRIV]],
// CHECK-NEXT: [[C_PRIV:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[REFC]],
// CHECK-NEXT: [[C_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[C_PRIV]],
// CHECK-NEXT: [[DIV:%.+]] = sdiv i{{[0-9]+}} [[C_VAL]], 1
// CHECK-NEXT: store i{{[0-9]+}} [[DIV]], i{{[0-9]+}}* [[C_PRIV]],
// CHECK-NEXT: [[E_PRIV:%.+]] = load [4 x i{{[0-9]+}}]*, [4 x i{{[0-9]+}}]** [[REFE]],
// CHECK-NEXT: [[E_PRIV_2:%.+]] = getelementptr inbounds [4 x i{{[0-9]+}}], [4 x i{{[0-9]+}}]* [[E_PRIV]], i{{[0-9]+}} 0, i{{[0-9]+}} 2
// CHECK-NEXT: store i32 1111, i32* [[E_PRIV_2]],
// CHECK-NEXT: ret void

// CHECK: define internal {{.*}}void [[TMAIN_MICROTASK]](i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}}, [2 x i32]* dereferenceable(8) %{{.+}}, i32* dereferenceable(4) %{{.+}}, [2 x [[S_INT_TY]]]* dereferenceable(8) %{{.+}}, [[S_INT_TY]]* dereferenceable(4) %{{.+}})
// CHECK: [[T_VAR_PRIV:%.+]] = alloca i{{[0-9]+}}, align 128
// CHECK: [[VEC_PRIV:%.+]] = alloca [2 x i{{[0-9]+}}], align 128
// CHECK: [[S_ARR_PRIV:%.+]] = alloca [2 x [[S_INT_TY]]], align 128
// CHECK: [[VAR_PRIV:%.+]] = alloca [[S_INT_TY]], align 128
// CHECK: store i{{[0-9]+}}* [[GTID_ADDR]], i{{[0-9]+}}** [[GTID_ADDR_ADDR:%.+]],

// CHECK: [[VEC_REF:%.+]] = load [2 x i{{[0-9]+}}]*, [2 x i{{[0-9]+}}]** %
// CHECK: [[T_VAR_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** %
// CHECK: [[S_ARR_REF:%.+]] = load [2 x [[S_INT_TY]]]*, [2 x [[S_INT_TY]]]** %
// CHECK: [[VAR_REF:%.+]] = load [[S_INT_TY]]*, [[S_INT_TY]]** %

// CHECK: [[T_VAR_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[T_VAR_REF]], align 128
// CHECK: store i{{[0-9]+}} [[T_VAR_VAL]], i{{[0-9]+}}* [[T_VAR_PRIV]], align 128
// CHECK: [[VEC_DEST:%.+]] = bitcast [2 x i{{[0-9]+}}]* [[VEC_PRIV]] to i8*
// CHECK: [[VEC_SRC:%.+]] = bitcast [2 x i{{[0-9]+}}]* [[VEC_REF]] to i8*
// CHECK: call void @llvm.memcpy.{{.+}}(i8* [[VEC_DEST]], i8* [[VEC_SRC]], i{{[0-9]+}} {{[0-9]+}}, i{{[0-9]+}} 128,
// CHECK: [[S_ARR_PRIV_BEGIN:%.+]] = getelementptr inbounds [2 x [[S_INT_TY]]], [2 x [[S_INT_TY]]]* [[S_ARR_PRIV]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
// CHECK: [[S_ARR_BEGIN:%.+]] = bitcast [2 x [[S_INT_TY]]]* [[S_ARR_REF]] to [[S_INT_TY]]*
// CHECK: [[S_ARR_PRIV_END:%.+]] = getelementptr [[S_INT_TY]], [[S_INT_TY]]* [[S_ARR_PRIV_BEGIN]], i{{[0-9]+}} 2
// CHECK: [[IS_EMPTY:%.+]] = icmp eq [[S_INT_TY]]* [[S_ARR_PRIV_BEGIN]], [[S_ARR_PRIV_END]]
// CHECK: br i1 [[IS_EMPTY]], label %[[S_ARR_BODY_DONE:.+]], label %[[S_ARR_BODY:.+]]
// CHECK: [[S_ARR_BODY]]
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_INT_TY_COPY_CONSTR:@.+]]([[S_INT_TY]]* {{.+}}, [[S_INT_TY]]* {{.+}}, [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR]]([[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: br i1 {{.+}}, label %{{.+}}, label %[[S_ARR_BODY]]
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_INT_TY_COPY_CONSTR]]([[S_INT_TY]]* [[VAR_PRIV]], [[S_INT_TY]]* {{.*}} [[VAR_REF]], [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR]]([[ST_TY]]* [[ST_TY_TEMP]])
// CHECK-NOT: call {{.*}}void @__kmpc_barrier(
// CHECK-DAG: call {{.*}} [[S_INT_TY_DESTR]]([[S_INT_TY]]* [[VAR_PRIV]])
// CHECK-DAG: call {{.*}} [[S_INT_TY_DESTR]]([[S_INT_TY]]*
// CHECK: ret void

#endif
#else
struct St {
  int a, b;
  St() : a(0), b(0) {}
  St(const St &) { }
  ~St() {}
  void St_func(St s[2], int n, long double vla1[n]) {
    double vla2[n][n] __attribute__((aligned(128)));
    a = b;
#pragma omp parallel firstprivate(s, vla1, vla2)
    vla1[b] = vla2[1][n - 1] = a = b;
  }
};

// ARRAY-LABEL: array_func
void array_func(float a[3], St s[2], int n, long double vla1[n]) {
  double vla2[n][n] __attribute__((aligned(128)));
// ARRAY: @__kmpc_fork_call(
// ARRAY-DAG: [[PRIV_S:%.+]] = alloca %struct.St*,
// ARRAY-DAG: [[PRIV_VLA1:%.+]] = alloca x86_fp80*,
// ARRAY-DAG: [[PRIV_A:%.+]] = alloca float*,
// ARRAY-DAG: [[PRIV_VLA2:%.+]] = alloca double*,
// ARRAY-DAG: store %struct.St* %{{.+}}, %struct.St** [[PRIV_S]],
// ARRAY-DAG: store x86_fp80* %{{.+}}, x86_fp80** [[PRIV_VLA1]],
// ARRAY-DAG: store float* %{{.+}}, float** [[PRIV_A]],
// ARRAY-DAG: store double* %{{.+}}, double** [[PRIV_VLA2]],
// ARRAY: call i8* @llvm.stacksave()
// ARRAY: [[SIZE:%.+]] = mul nuw i64 %{{.+}}, 8
// ARRAY: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %{{.+}}, i8* %{{.+}}, i64 [[SIZE]], i32 128, i1 false)
#pragma omp parallel firstprivate(a, s, vla1, vla2)
  s[0].St_func(s, n, vla1);
  ;
}

// ARRAY-LABEL: St_func
// ARRAY: @__kmpc_fork_call(
// ARRAY-DAG: [[PRIV_VLA1:%.+]] = alloca x86_fp80*,
// ARRAY-DAG: [[PRIV_S:%.+]] = alloca %struct.St*,
// ARRAY-DAG: [[PRIV_VLA2:%.+]] = alloca double*,
// ARRAY-DAG: store %struct.St* %{{.+}}, %struct.St** [[PRIV_S]],
// ARRAY-DAG: store x86_fp80* %{{.+}}, x86_fp80** [[PRIV_VLA1]],
// ARRAY-DAG: store double* %{{.+}}, double** [[PRIV_VLA2]],
// ARRAY: call i8* @llvm.stacksave()
// ARRAY: [[SIZE:%.+]] = mul nuw i64 %{{.+}}, 8
// ARRAY: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %{{.+}}, i8* %{{.+}}, i64 [[SIZE]], i32 128, i1 false)
#endif


