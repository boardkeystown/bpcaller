class FooBar:
    def __init__(
            self,
            a_int: int = 0,
            a_float: float = 0.0,
            a_double: float = 0.0,
            a_bool: bool = False,
            a_string: str = ''
            ) -> None:
        self.a_int = a_int
        self.a_float = a_float
        self.a_double = a_double
        self.a_bool = a_bool
        self.a_string = a_string
    def toStr() -> str: ...