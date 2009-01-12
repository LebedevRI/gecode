/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Christian Schulte <schulte@gecode.org>
 *
 *  Copyright:
 *     Christian Schulte, 2008
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

namespace Gecode { namespace Int { namespace Branch {

  /**
   * \brief %Branching descriptions storing position and values
   *
   * The maximal number of alternatives is defined by \a alt.
   */
  template<class ViewSel, class View>
  class PosValuesDesc : public PosDesc<ViewSel> {
  private:
    /// Information about position and minimum
    class PosMin {
    public:
      /// Start position of range
      unsigned int pos;
      /// Minmum of range
      int min;
    };
    /// Number of ranges
    unsigned int n;
    /// Values to assign to
    PosMin* pm;
  public:
    /// Initialize description for branching \a b, position \a p, view description \a viewd, and view \a x
    PosValuesDesc(const Branching& b, const Pos& p,
                  const typename ViewSel::Desc& viewd, View x);
    /// Return value to branch with for alternative \a a
    int val(unsigned int a) const;
    /// Report size occupied
    virtual size_t size(void) const;
    /// Deallocate
    virtual ~PosValuesDesc(void);
  };


  template<class ViewSel, class View>
  forceinline
  PosValuesDesc<ViewSel,View>::
  PosValuesDesc(const Branching& b, const Pos& p,
                const typename ViewSel::Desc& viewd, View x) 
    : PosDesc<ViewSel>(b,x.size(),p,viewd), n(0) {
    for (ViewRanges<View> r(x); r(); ++r)
      n++;
    pm = heap.alloc<PosMin>(n+1);
    unsigned int w=0;
    int i=0;
    for (ViewRanges<View> r(x); r(); ++r) {
      pm[i].min = r.min();
      pm[i].pos = w;
      w += r.width(); i++;
    }
    pm[i].pos = w;
  }
  
  template<class ViewSel, class View>
  forceinline int
  PosValuesDesc<ViewSel,View>::val(unsigned int a) const {
    PosMin* l = &pm[0];
    PosMin* r = &pm[n-1];
    while (true) {
      PosMin* m = l + (r-l)/2;
      if (a < m->pos) {
        r=m-1;
      } else if (a >= (m+1)->pos) {
        l=m+1;
      } else {
        return m->min + static_cast<int>(a - m->pos);
      }
    }
    GECODE_NEVER;
    return 0;
  }
  
  template<class ViewSel, class View>
  size_t
  PosValuesDesc<ViewSel,View>::size(void) const {
    return sizeof(PosValuesDesc<ViewSel,View>)+(n+1)*sizeof(PosMin);
  }
  
  template<class ViewSel, class View>
  PosValuesDesc<ViewSel,View>::~PosValuesDesc(void) {
    heap.free<PosMin>(pm,n+1);
  }
  

  template <class ViewSel, class View>
  forceinline
  ViewValuesBranching<ViewSel,View>::
  ViewValuesBranching(Space& home, ViewArray<typename ViewSel::View>& x,
                      ViewSel& vi_s)
    : ViewBranching<ViewSel>(home,x,vi_s) {}

  template <class ViewSel, class View>
  forceinline
  ViewValuesBranching<ViewSel,View>::
  ViewValuesBranching(Space& home, bool share, ViewValuesBranching& b)
    : ViewBranching<ViewSel>(home,share,b) {}

  template <class ViewSel, class View>
  Actor*
  ViewValuesBranching<ViewSel,View>::copy(Space& home, bool share) {
    return new (home)
      ViewValuesBranching<ViewSel,View>(home,share,*this);
  }

  template <class ViewSel, class View>
  const BranchingDesc*
  ViewValuesBranching<ViewSel,View>::description(Space& home) {
    Pos p = ViewBranching<ViewSel>::pos(home);
    View v(ViewBranching<ViewSel>::view(p).var());
    return new PosValuesDesc<ViewSel,View>
      (*this,p,viewsel.description(home),v);
  }

  template <class ViewSel, class View>
  ExecStatus
  ViewValuesBranching<ViewSel,View>
  ::commit(Space& home, const BranchingDesc& d, unsigned int a) {
    const PosValuesDesc<ViewSel,View>& pvd 
      = static_cast<const PosValuesDesc<ViewSel,View>&>(d);
    View v(ViewBranching<ViewSel>::view(pvd.pos()).var());
    viewsel.commit(home, pvd.viewdesc(), a);
    return me_failed(v.eq(home,pvd.val(a))) ? ES_FAILED : ES_OK;
  }

  template <class ViewSel, class View>
  Support::Symbol
  ViewValuesBranching<ViewSel,View>::ati(void) {
    return Reflection::mangle<ViewSel,View>
      ("Gecode::Int::ViewValuesBranching");
  }

  template <class ViewSel, class View>
  void
  ViewValuesBranching<ViewSel,View>::post
  (Space& home, Reflection::VarMap& vars,
   const Reflection::ActorSpec& spec) {
    spec.checkArity(1);
    ViewArray<typename ViewSel::View> x(home, vars, spec[0]);
    (void) new (home) ViewValuesBranching<ViewSel,View>(home, x);
  }

  template <class ViewSel, class View>
  Reflection::ActorSpec
  ViewValuesBranching<ViewSel,View>
  ::spec(const Space& home, Reflection::VarMap& m) const {
    Reflection::ActorSpec s(ati());
    return s << x.spec(home, m);
  }

  template <class ViewSel, class View>
  Reflection::BranchingSpec
  ViewValuesBranching<ViewSel,View>
  ::branchingSpec(const Space& home, 
                  Reflection::VarMap& m, const BranchingDesc& d) const {
    (void) home; (void) m;
    const PosValuesDesc<ViewSel,View>& pvd 
      = static_cast<const PosValuesDesc<ViewSel,View>&>(d);
    (void) pvd;
    Reflection::BranchingSpec bs(ati(), 2);
    bs[0] = view(pvd.pos()).spec(home, m);
    Reflection::IntArrayArg* ia = 
      Reflection::Arg::newIntArray(static_cast<int>(pvd.alternatives()));
    bs[1] = ia;
    for (int i=pvd.alternatives(); i--;)
      (*ia)[i] = pvd.val(i);
    return bs;
  }


}}

  template <>
  class ViewValuesBranchingToString<Int::IntView> {
  public:
    static std::ostream& toString(std::ostream& os,
                                  Space&, Reflection::VarMap&,
                                  const Reflection::BranchingSpec&,
                                  unsigned int alt) {
      return os << "ViewValuesBranching<IntView>" << alt << std::endl;
    }    
  };
  template <>
  class ViewValuesBranchingToString<Int::MinusView> {
  public:
    static std::ostream& toString(std::ostream& os,
                                  Space&, Reflection::VarMap&,
                                  const Reflection::BranchingSpec&,
                                  unsigned int alt) {
      return os << "ViewValuesBranching<MinusView>" << alt << std::endl;
    }    
  };

}

// STATISTICS: int-branch
