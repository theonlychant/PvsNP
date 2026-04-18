# FOL

P and NP are not objects - they are sets of languages. And "polynomial time" is not a single predicate - it's a statement quantifying over all polynomials. So we immediately need to ask: **what is our domain, and what can we say about it?**


Attempt to express it - layer by layer:
Layer 1 - The easy part, membership predicates:
```InP(L)   ≡  ∃M ∃k ∀x [ Decides(M, L, x) ∧ Halts(M, x, k, |x|) ]
InNP(L)  ≡  ∃M ∃k ∀x ∃c [ Verifies(M, L, x, c) ∧ Halts(M, x, k, |x|) ]
```

Already a problem. **Halts(M, x, k, |x|) means "M halts on x in at most n^k steps." The n^k part — quantifying over the exponent k - is arithmetic**. FOL over a pure complexity domain can't express this without importing the natural numbers into the structure.
Layer 2 - What we actually need:
To express P ≠ NP we need to say:
```∃L [ InNP(L) ∧ ¬InP(L) ]
```

That sentence looks like clean FOL. And it is - syntactically. The problem is what structure M makes it true or false. To evaluate InP(L) in M you need arithmetic in the background. So the sentence is FOL relative to a structure that already encodes polynomial time - which means the hard work is in defining M, not in writing the sentence.
Layer 3 - The quantifier depth problem:
Full unabbreviated expansion of InP(L):
```∃M ∃k ∀x ∀n [ |x| = n → ∃t [ Runs(M, x, t) ∧ t ≤ n^k ] ]
```

n^k requires *arithmetic*. t ≤ n^k with k universally quantified over integers is a Π₂ arithmetic statement. This puts us in the language of Peano Arithmetic, **not pure FOL. We've left first-order complexity language and entered arithmetic.**

What FOL genuinely can express - and what it cannot:

Statement               | Expressible in pure FOL?    |      Why? 
```∃L(InNP(L) ∧ ¬InP(L))```    Yes - - syntactically        But truth depends on arithmetic structure
```InP(L) for specific L```    No - - not purely            Requires bounding by n^k, needs arithmetic
```Cook-Levin reduction```     Partially                  The existence of f is FOL; poly-time of f is not
```SAT is NP-complete```     Partially                     Reduction structure yes; time bound no
```P ≠ NP as a tautology```    No                          Not valid in all FOL models
```P ≠ NP as a sentence```    Yes                          Writable but unprovable from FOL axioms alone 

The honest conclusion:
FOL can write ∃L(InNP(L) ∧ ¬InP(L)) - that is the FOL expression of P ≠ NP. But to give it meaning you need:

A structure M whose domain includes languages and machines
Arithmetic in M to interpret "polynomial time"
That arithmetic brings in Gödel - and with it the possibility that P ≠ NP is true but unprovable in your system

The real expressive power comes from bounded arithmetic - systems like Buss's S¹₂ - which are specifically designed to reason about polynomial time inside arithmetic. That's where FOL meets complexity theory in a technically serious way.

So the question becomes: do we want to

Write the FOL sentence ∃L(InNP(L) ∧ ¬InP(L)) precisely with a fully defined vocabulary and structure M - making theory2.c and theory2.md about expression rather than proof, or
Jump straight to bounded arithmetic (S¹₂) as the right language and make that Attempt 2 - since that's where the real action is?