# P vs NP — Attempt 2: First-Order Logic Expression

**Project:** Swirly Crop — P vs NP Deductive Series  
**File:** `theory2.c`  
**Built on:** `deduce.c`, `theory1.c`  
**Status:** `EXPRESSION ACHIEVED / PROOF IMPOSSIBLE IN PURE FOL`  
**Series position:** Attempt 2 of N

---

## What This Attempt Does

Attempt 1 tried to *prove* P ≠ NP via diagonalization and hit the
relativization barrier. Attempt 2 steps back and asks a more precise
question first:

> **Can FOL even express P ≠ NP as a well-formed sentence?**

The answer is: **yes, syntactically** - but only relative to a structure
that already contains arithmetic. The attempt builds that expression
precisely and evaluates it in a concrete structure M.

---

## The Vocabulary

Following the textbook (Ch. 14 - Introduction to FOL):

### Sorts
| Sort | Meaning |
|---|---|
| `Language` | A language L ⊆ Σ* |
| `Machine` | A Turing machine M |
| `Nat` | A natural number (time bounds) |
| `String` | An input string x ∈ Σ* |
| `Cert` | A certificate/witness c |

### Predicate Symbols
| Symbol | Arity | Meaning |
|---|---|---|
| `InP(L)` | 1 | L ∈ P |
| `InNP(L)` | 1 | L ∈ NP |
| `Decides(M,L,x,k)` | 4 | M decides x∈L in n^k steps |
| `Verifies(M,L,x,c,k)` | 5 | M verifies c for x∈L in n^k steps |
| `PolyTime(M,k)` | 2 | M runs in O(n^k) |
| `ReducesTo(L1,L2,f,k)` | 4 | poly-time reduction L1 → L2 |
| `Member(x,L)` | 2 | x ∈ L |
| `Leq(a,b)` | 2 | a ≤ b (arithmetic) |

### Function Symbols
| Symbol | Meaning |
|---|---|
| `\|x\|` | Length of string x |
| `n^k` | Arithmetic power — **requires PA** |
| `f ∘ g` | Function composition |

### Constant Symbols
`SAT`, `3COLOR`, `HALT`, `0`, `1`

---

## The FOL Sentence for P ≠ NP

### Surface form

```
∃L [ InNP(L) ∧ ¬InP(L) ]
```

This reads: *there exists a language that is in NP but not in P.*  
Since P ⊆ NP is known, this is equivalent to P ≠ NP.

### Expanded form (predicates unpacked)

```
∃L [
  ( ∃M_v ∃k ∀x ∃c
      Verifies(M_v, x, c) ∧ Steps(M_v, x, c) ≤ |x|^k )
  ∧
  ¬( ∃M_d ∃k ∀x
      Decides(M_d, x) ∧ Steps(M_d, x) ≤ |x|^k )
]
```

### Quantifier depth

```
∃L          — language quantifier          [sort: Language]
├ ∃M_v      — verifier machine             [sort: Machine ]
├ ∃k        — polynomial exponent          [sort: Nat     ]
├ ∀x        — universal input              [sort: String  ]
├ ∃c        — certificate witness          [sort: Cert    ]
├ ∃M_d      — decider machine (negated)    [sort: Machine ]
└ ∃k'       — polynomial exponent (negated)[sort: Nat     ]
```

This is a `Σ²₁` sentence in the arithmetic hierarchy — existential
over languages, with mixed quantifiers inside over machines and inputs.

---

## The Structure M

A structure M = (Domain, Interpretation) where:

**Domain:**
- Languages: `{SAT, 3COLOR, HALT}` — with ground truth `in_P`, `in_NP` flags
- Machines: `{poly_decider, np_verifier}` — with runtime exponents
- Arithmetic component: present (PA)

**Interpretation of InP(L):**
```
InP(L) ≡ ∃M ∃k ∀x [ Decides(M,L,x) ∧ Steps(M,x) ≤ |x|^k ]
```
Evaluated via ground truth in mock structure. Flags `arithmetic_required = true`.

**Interpretation of InNP(L):**
```
InNP(L) ≡ ∃M_v ∃k ∀x [ x∈L → ∃c [ |c|≤|x|^k ∧ Verifies(M_v,x,c) ] ]
```
Same — arithmetic required for certificate bound.

---

## The Satisfaction Relation

Defined recursively on formula structure, following the textbook:

```
M ⊨ P(t₁,...,tₙ)[v]   iff  (t₁^M,...,tₙ^M) ∈ P^M
M ⊨ ¬φ[v]             iff  M ⊭ φ[v]
M ⊨ φ∧ψ[v]            iff  M ⊨ φ[v]  and  M ⊨ ψ[v]
M ⊨ ∃x.φ[v]           iff  ∃d ∈ Domain: M ⊨ φ[v[x↦d]]
M ⊨ ∀x.φ[v]           iff  ∀d ∈ Domain: M ⊨ φ[v[x↦d]]
```

Each step tracks `arithmetic_used` — whether PA was required to
evaluate the subformula. This flag propagates upward through the tree.

---

## Evaluation Result

Running `theory2.c`:

```
M ⊨ ∃L[InNP(L) ∧ ¬InP(L)]  is  TRUE
Arithmetic used: YES — PA required
Witness: L = 3COLOR  (InNP(3COLOR) = true, InP(3COLOR) = false)
```

The sentence is **true in our structure M**. The witness is `3COLOR` —
a language assumed to be in NP but not in P (consistent with the
belief that P ≠ NP, encoded in the ground truth of the mock domain).

The `arithmetic_required` flag is `true` throughout — PA is used at
every step where a time bound is evaluated.

---

## The Wall — Three Layers

### Layer 1: Arithmetic

`InP` and `InNP` cannot be interpreted without `|x|^k`. This pulls
in Peano Arithmetic (or at minimum Buss's bounded arithmetic S¹₂).
The sentence lives in **FOL+PA**, not pure FOL.

Pure FOL is too weak to express polynomial time bounds. The `n^k`
function symbol with universally quantified `k` requires arithmetic
induction to reason about correctly.

### Layer 2: The Model Problem

By Gödel's completeness theorem:

```
⊢ φ   iff   ⊨ φ  (φ valid in all models)
```

P ≠ NP is **not valid in all models** of our complexity axioms.
Baker-Gill-Solovay showed that relative to some oracles P = NP —
which means in those oracle models, `∃L[InNP(L) ∧ ¬InP(L)]` is
**false**. Therefore the sentence is not a FOL tautology. FOL
completeness cannot derive it from standard axioms.

### Layer 3: Independence

Even in FOL+PA, P ≠ NP may be **independent** of the axiom system
— true but unprovable, analogous to the independence of the
Continuum Hypothesis from ZFC. This possibility cannot be ruled
out without resolving the problem itself.

---

## What Was Gained

Compared to Attempt 1 (diagonalization), Attempt 2 achieves:

- A precise, well-typed **FOL vocabulary** for complexity theory
- A formally defined **structure M** with a satisfaction relation
- The **exact boundary** between pure FOL and arithmetic
- Syntactic confirmation that `∃L[InNP(L) ∧ ¬InP(L)]` is well-formed
- Clear identification of the **model-theoretic obstacle**
- A working implementation of `M ⊨ φ[v]` in C

The sentence is written. The structure is defined. The evaluation runs.
What cannot be done is *proving* the sentence from axioms alone — the
model problem blocks it.

---

## Natural Deduction Rules Used

Same rules as `deduce.c` and `theory1.c`, with the addition of:

**∃-Elim** (used implicitly in satisfaction):
```
∃x.φ(x)    ∀x[ φ(x) → ψ ]
─────────────────────────────  ∃Elim
            ψ
```

**Substitution** (valuation extension):
```
φ[x ↦ t]
```
The recursive `satisfies()` function implements this via the
`extend(v, var, sort, idx)` call before evaluating the body of
each quantified formula.

---

## Verdict

| Property | Result |
|---|---|
| FOL sentence well-formed | Yes |
| Structure M defined | Yes |
| Satisfaction relation M ⊨ φ | Implemented and evaluated |
| Sentence true in M | Yes (witness: 3COLOR) |
| Sentence provable from FOL axioms | No |
| Barrier | Model-theoretic (BGS) + arithmetic ceiling |
| Collapses at | Gödel completeness + oracle models |

---

## What Attempt 3 Will Try

Move into **bounded arithmetic S¹₂** (Buss 1986) — a formal system
specifically calibrated to polynomial time. Unlike full PA, S¹₂:

- Can express `InP` and `InNP` without unbounded induction
- Has proof complexity directly connected to P vs NP
- Is a *non-relativizing* system in the right sense

The key result: if P ≠ NP, then the propositional tautologies
encoding this fact have no polynomial-length S¹₂ proofs. Conversely,
a short S¹₂ proof of the tautology would collapse the polynomial
hierarchy. Attempt 3 will encode this structure and push as far as
the bounded arithmetic framework allows.

---

*Swirly Crop(chant[ψαλμός]) - P vs NP Deductive Series*