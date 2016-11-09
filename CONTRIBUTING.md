# Coding Guidelines

## C++ Coding Guidelines

### Indenting and Spacing

- Indenting must be done with tab characters.
- Spacing must be done with space characters.
- The following indent style shall be followed (Allman style):
```
if(x == y)
{
    statement;
}
```

### Naming

- Class member method names shall begin with a upper case character (ex.: `int DoSomething();`).
- Class member variable names shall be prefixed by `m_` (ex.: `int m_member = 0;`).

### Constructors and Destructors

- Empty ctors and dtors definitions are not permitted.
- Use direct member initialization when possible.
- If an empty default ctor is needed, it must be declared as `default` in the class declaration.
- Empty virtual dtors must be marked as `default` in the class declaration.

### Override Keyword

All methods overriding a virtual method from a base class shall be annotated with `override` and must not be marked as `virtual`.

**Exception**: dtors shall be marked as `virtual` and not `override`.

### IOP HLE Modules

- **Module Methods**: All parameters for must be passed as `uint32`. Pointer parameters must also be passed as `uint32` and the name of the parameter must be prepended with `Ptr` (ie. `stringPtr`). When used, return values must be `int32`.

## Objective-C Coding Guidelines

### Naming

- Interface member method names shall begin with a lower case character (ex.: `-(int)doSomething;`).
