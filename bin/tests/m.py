from typing import List, Union, Any, Dict

class Cell:
    def __init__(self, value: Union[int, float, str] = 0.0, default: Union[int, float, str] = 0.0):
        self.v = value
        self.d = default
        self.use_default = True
        self.immutable = False
    
    @property
    def value(self):
        if not self.immutable and self.use_default:
            v = self.v
            self.v = self.d
            return v
        return self.v
    
    @value.setter
    def value(self, v):
        if not self.immutable:
            self.v = v
    @property
    def default(self): return self.d
    @default.setter
    def default(self, v): self.d = v

    def __copy__(self): 
        o = Cell(self.v, self.d)
        o.use_default = self.use_default
        o.immutable = self.immutable
        return o

    def copy(self):
        return self.__copy__()

class CellWrapper:
    def __init__(self, value: Union[int, float, str] = 0.0, default: Union[int, float, str] = 0.0):
        self.cell = Cell(value, default)

    @property
    def value(self) -> Union[int, float, str]: return self.cell.value
    @value.setter
    def value(self, v: Union[int, float, str]): self.cell.value = v
    
    @property
    def default(self) -> Union[int, float, str]: return self.cell.default
    @default.setter
    def default(self, v: Union[int, float, str]): self.cell.default = v

    @property
    def use_default(self) -> bool: return self.cell.use_default
    @use_default.setter
    def use_default(self, v: bool): self.cell.use_default = v

    @property
    def immutable(self) -> bool: return self.cell.immutable
    @immutable.setter
    def immutable(self, v: bool): self.cell.immutable = v

    def entangle(self, o: Union["CellWrapper", "Cell"]):
        if isinstance(o, CellWrapper):
            self.cell = o.cell
        elif isinstance(o, Cell):
            self.cell = o
        else:
            raise AttributeError("object cannot be entangled")
    
    def untangle(self):
        c = self.cell.copy()
        self.cell = c

    def __copy__(self) -> "CellWrapper":
        o = CellWrapper()
        o.cell = self.cell.copy()
        return o

    def copy(self) -> "CellWrapper":
        return self.__copy__()
    
class Row:
    cells: Dict[str, CellWrapper]

    def __init__(self, headers: List[str]):
        object.__setattr__(self, "cells", {key: CellWrapper() for key in headers})

    def __setattr__(self, name: str, value: Any):
        cells = object.__getattribute__(self, "cells")
        if name in cells:
            cells[name].value = value
        else:
            object.__setattr__(self, name, value)

    def __getattribute__(self, name: str):
        if name in ("cells", "__dict__", "__class__"):
            return object.__getattribute__(self, name)

        cells = object.__getattribute__(self, "cells")
        if name in cells:
            return cells[name].value
        return object.__getattribute__(self, name)

    def __getitem__(self, key: str):
        return self.cells[key].value

    def __setitem__(self, key: str, value: Any):
        if key in self.cells:
            self.cells[key].value = value
        else:
            self.cells[key] = CellWrapper(value)
    
    def keys(self) -> List[str]:
        return list(self.cells.keys())
    
    def values(self) -> List["CellWrapper"]:
        return list(self.cells.values())
    
    def items(self):
        return self.cells.items()
    
    def __len__(self) -> int:
        return len(self.cells)
    
    def __copy__(self) -> "Row":
        r = Row(self.keys())
        for k, v in self.cells.items():
            r.cells[k] = v.copy()
        return r
    
    def toStr(self) -> str:
        v = ", ".join([f"{k} = {v.cell.v}" for k,v in self.cells.items()])
        return v

    def __str__(self) -> str:
        return f'{type(self).__name__}( {self.toStr()} )'
    
    def __iter__(self):
        return iter(self.cells)
    
    def copy(self) -> "Row":
        return self.__copy__()

    def getCell(self, key: str, default=None) -> "CellWrapper":
        return self.cells.get(key, default)
    
    def setDefault(
                self, 
                key: str = None,
                default: Union[int,float,str] = None, 
                use_default : bool = True
                ):
        if key:
            cell = self.cells.get(key)
            if default: cell.default = default
            cell.use_default = use_default
        else:
            for _, cell in self.cells.items():
                if default: cell.default = default
                cell.use_default = use_default

    def setImmutable(self, key: str, immutable: bool):
        self.cells.get(key).immutable = immutable


class Backend:
    def __init__(self, filename: str):
        import atexit

        self.file = open(filename, 'w')
        atexit.register(self.close)
        self.isWriting = False
        self.first_write = False
    
    @property
    def name(self) -> str:
        return self.file.name

    def w_header(self, l: List[str]):
        if not self.isWriting:
            nl = '\n'
            if not self.first_write:
                self.first_write = True
                nl = ''
            self.file.write(f'{nl}{",".join(l)}')
        else:
            raise RuntimeError(
                f"file '{self.name}' started writing rows"
            )
    
    def w_row(self, rows: List[Union[int,float,str]]):
        self.isWriting = True
        self.file.write(f'\n{",".join([f"{i}" for i in rows])}')
    
    def close(self):
        if not self.file.closed:
            self.file.close()
    def __del__(self):
        self.close()

class Record:
    def __init__(self,
            headers : List[str],
            backends : List[Backend], 
            index : int = None):
        self.row = Row(headers)
        self._index = index
        self._backends = backends
        self._w_header()
    
    def _w_header(self):
        for b in self._backends:
            if self._index is None:
                b.w_header(self.row.keys())
            else:
                b.w_header(["index", *self.row.keys()])
    
    def _w_row(self):
        for b in self._backends:
            if self._index is None:
                b.w_row([i.value for i in self.row.values()])
            else:
                b.w_row([self._index, *[i.value for i in self.row.values()]])
    
    def write(self,*args, **kwargs):
        if args:
            al = list(args) 
            rk = self.row.keys()[0:len(al)]
            if len(al) > len(rk):
                raise AttributeError(
                    f"provided args '{args}' larger than headers '{rk}'"
                )
            for k,v in list(zip(rk, al)):
                self.row[k] = v
        for k,v in kwargs.items():
            c = self.row.cells.get(k, None)
            if c: c.value = v
            else: raise AttributeError(f"Unknown key '{k}'")
        
        self._w_row()

b = Backend('out.txt')
r1 = Record(["a","b","c"], [b])
r1.row.setDefault("c",use_default=False)

r1.write(1,2,3)

print(r1.row)

# r1.write(b=123,c=444)
# r1.write()
# a = r1.row.getCell("a")
# a.value = 999
# a.immutable = True
# r1.write(1,2,3)
# r2 = Record(['beans','candy'], [Backend('out2.txt')])
# r2.row.cells.get('beans').entangle(r1.row.getCell("c"))
# r2.write()