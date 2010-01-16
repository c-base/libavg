// Original code by Hans Meine <meine <at> kogs1.informatik.uni-hamburg.de>:
// Subject: Re: Raw constructor (i.e. combination of make_constructor and raw_function)
// Newsgroups: gmane.comp.python.c++
// Date: 2005-11-17 18:43:48 GMT
// No copyright statement found. Presumed to be in the public domain.

#ifndef RAW_CONSTRUCTOR_HPP
#define RAW_CONSTRUCTOR_HPP

#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/make_constructor.hpp>
#include "boost/python/detail/api_placeholder.hpp" // python::len

namespace boost { namespace python {

namespace detail {

  template <class F>
  struct raw_constructor_dispatcher
  {
      raw_constructor_dispatcher(F f)
     : f(make_constructor(f)) {}

      PyObject* operator()(PyObject* args, PyObject* keywords)
      {
          borrowed_reference_t* ra = borrowed_reference(args);
          object a(ra);
          return incref(
              object(
                  f(
                      object(a[0])
                    , object(a.slice(1, len(a)))
                    , keywords ? dict(borrowed_reference(keywords)) : dict()
                  )
              ).ptr()
          );
      }

   private:
      object f;
  };

} // namespace detail

template <class F>
object raw_constructor(F f, std::size_t min_args = 0)
{
    return detail::make_raw_function(
        objects::py_function(
            detail::raw_constructor_dispatcher<F>(f)
          , mpl::vector2<void, object>()
          , min_args+1
          , (std::numeric_limits<unsigned>::max)()
        )
    );
}

}} // namespace boost::python

#endif // RAW_CONSTRUCTOR_HPP
