#pragma once
#include <vector>
#include <algorithm>
#include "_main.hxx"
#include "vertices.hxx"
#include "edges.hxx"
#include "csr.hxx"
#include "transpose.hxx"
#include "components.hxx"
#include "sort.hxx"
#include "dynamic.hxx"
#include "pagerank.hxx"
#include "pagerankOmp.hxx"
#include "pagerankMonolithicOmp.hxx"

using std::vector;
using std::swap;




// PAGERANK-LOOP
// -------------

template <class T, class J>
float pagerankComponentwiseOmpLoop(vector<T>& a, vector<T>& r, vector<T>& c, const vector<T>& f, const vector<int>& vfrom, const vector<int>& efrom, int i, const J& ns, int N, T p, T E, int L, int EF) {
  float t = 0;
  for (int n : ns) {
    if (n<=0) { i += -n; continue; }
    T np = T(n)/N, En = EF<=2? E*n/N : E;
    t += pagerankMonolithicOmpLoop(a, r, c, f, vfrom, efrom, i, n, N, p, En, L, EF);
    swap(a, r);
    i += n;
  }
  swap(a, r);
  return t;
}




// PAGERANK (STATIC / INCREMENTAL)
// -------------------------------

// Find pagerank using a single thread (pull, CSR).
// @param x  original graph
// @param xt transpose graph (with vertex-data=out-degree)
// @param q  initial ranks (optional)
// @param o  options {damping=0.85, tolerance=1e-6, maxIterations=500}
// @returns {ranks, iterations, time}
template <class G, class H, class T=float>
PagerankResult<T> pagerankComponentwiseOmp(const G& x, const H& xt, const vector<T> *q, const PagerankOptions<T>& o, const PagerankData<G>& D) {
  const auto& cs = D.components;
  const auto& b  = D.blockgraph;
  int  N  = xt.order();  if (N==0) return PagerankResult<T>::initial(xt, q);
  auto ds = topologicalComponentsFrom(cs, b);
  auto gs = joinUntilSize<int>(ds, o.minCompute);
  auto ns = transformIter(gs, [&](const auto& c) { return c.size(); });
  auto ks = join<int>(gs);
  return pagerankOmp(xt, ks, 0, ns, pagerankComponentwiseOmpLoop<T, decltype(ns)>, q, o);
}
template <class G, class H, class T=float>
PagerankResult<T> pagerankComponentwiseOmp(const G& x, const H& xt, const vector<T> *q=nullptr, const PagerankOptions<T>& o={}) {
  auto cs = components(x, xt);
  auto b  = blockgraph(x, cs);
  return pagerankComponentwiseOmp(x, xt, q, o, PagerankData<G>(cs, b));
}
template <class G, class T=float>
PagerankResult<T> pagerankComponentwiseOmp(const G& x, const vector<T> *q=nullptr, PagerankOptions<T> o={}) {
  auto xt = transposeWithDegree(x);
  return pagerankComponentwiseOmp(x, xt, q, o);
}




// PAGERANK (DYNAMIC)
// ------------------

template <class G, class H, class T=float>
PagerankResult<T> pagerankComponentwiseOmpDynamic(const G& x, const H& xt, const G& y, const H& yt, const vector<T> *q, const PagerankOptions<T>& o, const PagerankData<G>& D) {
  const auto& cs = D.components;
  const auto& b  = D.blockgraph;
  int  N  = yt.order();                                 if (N==0) return PagerankResult<T>::initial(yt, q);
  auto ds = topologicalComponentsFrom(cs, b);
  auto [is, n] = dynamicComponentIndices(x, y, ds, b);  if (n==0) return PagerankResult<T>::initial(yt, q);
  auto gs = joinAtUntilSize<int>(ds, sliceIter(is, 0, n), o.minCompute);
  auto ns = transformIter(gs, [&](const auto& g) { return g.size(); });
  auto ks = join<int>(gs); joinAt(ks, ds, sliceIter(is, n));
  return pagerankOmp(yt, ks, 0, ns, pagerankComponentwiseOmpLoop<T, decltype(ns)>, q, o);
}
template <class G, class H, class T=float>
PagerankResult<T> pagerankComponentwiseOmpDynamic(const G& x, const H& xt, const G& y, const H& yt, const vector<T> *q=nullptr, const PagerankOptions<T>& o={}) {
  auto cs = components(y, yt);
  auto b  = blockgraph(y, cs);
  return pagerankComponentwiseOmpDynamic(x, xt, y, yt, q, o, PagerankData<G>(cs, b));
}
template <class G, class T=float>
PagerankResult<T> pagerankComponentwiseOmpDynamic(const G& x, const G& y, const vector<T> *q=nullptr, PagerankOptions<T> o={}) {
  auto xt = transposeWithDegree(x);
  auto yt = transposeWithDegree(y);
  return pagerankComponentwiseOmpDynamic(x, xt, y, yt, q, o);
}
