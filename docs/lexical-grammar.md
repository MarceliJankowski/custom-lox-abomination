# CLA Lexical Grammar

This document details CLA's lexical grammar.<br>
It lists token kinds in UPPERCASE and groups them under sections.

There are two types of token kind definitions:

1. `TOKEN_KIND -> RegExp`
2. `TOKEN_KIND := Text`

In these definitions:

- `TOKEN_KIND` - denotes token kind being defined.
- `' -> '` - delimiter specifying that what comes after is a Regular Expression.
- `' := '` - delimiter specifying that what comes after is a literal text.
- `RegExp` - Regular Expression to be matched against.
- `Text` - literal text to be matched against.

## Lexical Resolution Rules

1. Tokens under **Ignored Tokens** section are ignored.
2. Token kind with a longest matching lexeme is selected.
3. Reserved identifiers take precedence over `IDENTIFIER` token kind.

### Ignored Tokens

```
WHITESPACE -> [ \t\r\n]+
COMMENT -> #[^\n]*
```

### Literals

```
NUMBER -> [0-9]+(\.[0-9]+)?
STRING -> "([^"\\\n]|\\[\\"abfnrtv])*"
IDENTIFIER -> [a-zA-Z_][a-zA-Z0-9_]*
```

### Reserved Identifiers

```
PRINT := print
TRUE := true
FALSE := false
NIL := nil
```

### Single-character Tokens

```
PLUS := +
MINUS := -
STAR := *
SLASH := /
PERCENT := %
BANG := !
LESS := <
EQUAL := =
GREATER := >
DOT := .
COMMA := ,
COLON := :
SEMICOLON := ;
QUESTION := ?
OPEN_PAREN := (
CLOSE_PAREN := )
OPEN_CURLY_BRACE := {
CLOSE_CURLY_BRACE := }
```

### Multi-character Tokens

```
DOT_DOT := ..
EQUAL_EQUAL := ==
BANG_EQUAL := !=
LESS_EQUAL := <=
GREATER_EQUAL := >=
```
