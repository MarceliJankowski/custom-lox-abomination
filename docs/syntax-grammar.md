# CLA Syntax Grammar

This document details CLA's syntax grammar in a BNF notation.

Terminal symbols in production bodies refer to token kinds defined in [lexical-grammar](lexical-grammar.md).

## BNF

```
<program> ::= ε
            | <stmt-list>

<stmt-list> ::= <stmt>
              | <stmt><stmt-list>

<stmt> ::= <print-stmt>
         | <expr-stmt>

<print-stmt> ::= PRINT <expr> SEMICOLON

<expr-stmt> ::= <expr> SEMICOLON

<expr> ::= <equality-expr>

<equality-expr> ::= <comparison-expr>
                  | <comparison-expr> EQUAL_EQUAL <comparison-expr>
                  | <comparison-expr> BANG_EQUAL <comparison-expr>

<comparison-expr> ::= <term-expr>
                    | <term-expr> LESS <term-expr>
                    | <term-expr> LESS_EQUAL <term-expr>
                    | <term-expr> GREATER <term-expr>
                    | <term-expr> GREATER_EQUAL <term-expr>

<term-expr> ::= <factor-expr>
              | <term-expr> PLUS <factor-expr>
              | <term-expr> MINUS <factor-expr>

<factor-expr> ::= <unary-expr>
                | <factor-expr> STAR <unary-expr>
                | <factor-expr> SLASH <unary-expr>
                | <factor-expr> PERCENT <unary-expr>

<unary-expr> ::= MINUS <unary-expr>
               | BANG <unary_expr>
               | <concatenation-expr>

<concatenation-expr> ::= <primary-expr>
                | <primary-expr> DOT_DOT <concatenation-expr>

<primary-expr> ::= NIL
                 | NUMBER
                 | STRING
                 | TRUE
                 | FALSE
                 | OPEN_PAREN <expr> CLOSE_PAREN
```
