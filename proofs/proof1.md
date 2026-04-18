The statement we're proving:
If SAT ∈ P, then for all L ∈ NP, L ∈ P.
In logical form:
(SAT ∈ P) → (∀L. L ∈ NP → L ∈ P)

The premises we're allowed to assume (these are proven mathematics):

Reduction premise: ∀L ∈ NP, ∃ polynomial-time reduction f such that x ∈ L ↔ f(x) ∈ SAT
Composition premise: If g runs in polynomial time and h runs in polynomial time, then h∘g runs in polynomial time
SAT hypothesis: SAT ∈ P (this is the assumption we'll discharge at the end via →Intro)


The derivation:
```[SAT ∈ P]¹        ∀L∈NP. ∃f_L poly-time. x∈L ↔ f_L(x)∈SAT
─────────────────────────────────────────────────────────────
                    Pick arbitrary L ∈ NP
                           │
                    ∃f_L poly-time reduction from L to SAT      [SAT ∈ P]¹
                           │                                          │
                    f_L runs in poly-time              A_SAT runs in poly-time
                    ─────────────────────────────────────────────────────────
                                  Composition premise
                              A_SAT ∘ f_L runs in poly-time
                                           │
                              x ∈ L ↔ f_L(x) ∈ SAT ↔ A_SAT(f_L(x)) accepts
                                           │
                                    A_SAT ∘ f_L decides L
                                           │
                                      ∧Intro
                         (A_SAT ∘ f_L decides L) ∧ (A_SAT ∘ f_L poly-time)
                                           │
                                      →Intro (L ∈ P by definition)
                                        L ∈ P
                                           │
                              L was arbitrary, so ∀L ∈ NP. L ∈ P
                    ¹─────────────────────────────────────────────────────
                         SAT ∈ P → ∀L ∈ NP. L ∈ P          →Intro, discharging [SAT ∈ P]¹
```

# What each rule is doing
 - →Intro at the end: we assumed SAT ∈ P, derived the conclusion, then discharged the assumption exactly like the textbook page you showed. The superscript ¹ marks where the assumption is discharged.

 - ∧Intro: combining "A_SAT ∘ f_L decides L" with "it runs in polynomial time" into a single conjunction both conditions needed for L ∈ P.

 - ∀-Elim / ∀-Intro: we picked an arbitrary L from NP, derived L ∈ P, then generalized back to all L.

 - The biconditional chain: x ∈ L ↔ f_L(x) ∈ SAT is the reduction; A_SAT(f_L(x)) accepts ↔ f_L(x) ∈ SAT is the SAT solver - - chaining them gives a decider for L.

What this proof does NOT do:
It does not prove SAT ∈ P. It proves the implication the logical skeleton that makes SAT the right target. The assumption SAT ∈ P is discharged, not established.

These are the certain premises:

```Premise 1 - NP Reducibility (Cook-Levin core):

For every language L ∈ NP, there exists a polynomial-time computable function f_L such that for all inputs x:
x ∈ L ↔ f_L(x) ∈ SAT

This is the definition of NP-completeness of SAT. It was proven by Cook (1971) and independently Levin (1973). It is not an assumption — it is established mathematics.

Premise 2 - Polynomial Composition Closure:

If f runs in time O(n^j) and g runs in time O(n^k), then g∘f runs in time O(n^(jk)).

Polynomial time is closed under composition. If you feed a polynomial-time output into a polynomial-time algorithm, the whole pipeline is still polynomial. This follows directly from the definition of big-O and polynomial arithmetic.

Premise 3 - SAT Hypothesis (discharged assumption):

[SAT ∈ P]¹

This is the assumption we introduce and later discharge via →Intro. It is not asserted as true — it is the antecedent of the conditional we are proving. Marking it with superscript ¹ tracks where it enters and where it gets discharged.

Premise 4 - Definition of P membership:

A language L ∈ P if and only if there exists a Turing machine M and a polynomial p such that M decides L and for all inputs x, M halts in at most p(|x|) steps.

This is just the definition of P. We need it stated explicitly because the derivation concludes L ∈ P by exhibiting the machine A_SAT ∘ f_L and verifying both conditions — correctness and runtime.

Premise 5 - Correctness of the composed decider:

If A_SAT decides SAT correctly and f_L is a correct reduction, then A_SAT(f_L(x)) accepts ↔ x ∈ L.

This is the biconditional chain in the derivation. It follows by substitution from Premises 1 and 3 together - no new content, just making the logical step explicit.
```

