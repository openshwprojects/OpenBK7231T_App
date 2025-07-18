#ifndef __MINMAX_H__
#define __MINMAX_H__

template<typename A,typename B> A min(A a,B b)
{
  return a<b?a:b;
}

template<typename A> A min(A a,A b,A c)
{
  return min(min(a,b),c);
}

template<typename A> A min(A a,A b,A c,A d)
{
  return min(min(a,b,c),d);
}

template<typename A> A min(A a,A b,A c,A d,A e)
{
  return min(min(a,b,c,d),e);
}

template<typename A> A min(A a,A b,A c,A d,A e,A f)
{
  return min(min(a,b,c,d,e),f);
}

template<typename A> A min(A a,A b,A c,A d,A e,A f,A g)
{
  return min(min(a,b,c,d,e,f),g);
}


template<typename A,typename B> A max(A a,B b)
{
  return a>b?a:b;
}


#endif //__MINMAX_H__