class Writer:
    def __init__(self, file: str = 'out.txt'):
        self.f = open(file, 'w')
    
    def print(self, *args, **kwargs):
        print(*args, file=self.f)
    
    def __del__(self):
        if not self.f.closed:
            self.f.write('goodbye!!!')
            self.f.close()

print('hello from foo.py')
w = Writer()
w.print('add line to file')
w.print('adding another line')

count = 0
def test():
    global count
    count += 1
    print('you called test from foo.py', "#", count)

def test2(value):
    print(f"passed into test2: {value=}")

def test3_str():
    return "hello from python"
def test3_float():
    return 123.123
def test3_int():
    return 42
def test3_bool():
    return True

from typing import TYPE_CHECKING, List
if TYPE_CHECKING:
    from my_cpp_module import FooBar

def test4_foobar(foobar :"FooBar"):
    print("in python", foobar.toStr())
    foobar.a_int += 1
    foobar.a_string += 'haha '

def test4_foobar_list(amount: int) -> List["FooBar"]:
    print(f'{amount=}')
    from my_cpp_module import FooBar
    l = [FooBar(a_int= i, a_string= f'#{i+1}') for i in range(0, amount)]
    return l

def test4_to_foobar_list(fbs: List["FooBar"]):
    for f in fbs:
        print('in python ', f.toStr())