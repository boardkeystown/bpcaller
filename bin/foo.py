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