
def read_input(path):
    f = open(path, 'r')
    state = []
    part2data = []
    line = f.readline()
    while line:
        if line[0] == 'B':
            before = [int(line[9]), int(line[12]), int(line[15]), int(line[18])]
            line = f.readline().strip().split()
            ops = tuple((int(x) for x in line))
            line = f.readline()
            after = [int(line[9]), int(line[12]), int(line[15]), int(line[18])]
            state.append((before, ops, after))
            line = f.readline()
        else:
            op = line.strip().split()
            if op:
                part2data.append(map(int,op))
        line = f.readline()
    f.close()
    return state, part2data

def addr(reg, A, B, C):
    reg[C] = reg[A] + reg[B]
def addi(reg, A, B, C):
    reg[C] = reg[A] + B
def mulr(reg, A, B, C):
    reg[C] = reg[A] * reg[B]
def muli(reg, A, B, C):
    reg[C] = reg[A] * B
def banr(reg, A, B, C):
    reg[C] = reg[A] & reg[B]
def bani(reg, A, B, C):
    reg[C] = reg[A] & B
def borr(reg, A, B, C):
    reg[C] = reg[A] | reg[B]
def bori(reg, A, B, C):
    reg[C] = reg[A] | B
def setr(reg, A, B, C):
    reg[C] = reg[A]
def seti(reg, A, B, C):
    reg[C] = A
def gtir(reg, A, B, C):
    if A > reg[B]:
        reg[C] = 1
    else:
        reg[C] = 0
def gtri(reg, A, B, C):
    if reg[A] > B:
        reg[C] = 1
    else:
        reg[C] = 0
def gtrr(reg, A, B, C):
    if reg[A] > reg[B]:
        reg[C] = 1
    else:
        reg[C] = 0
def eqir(reg, A, B, C):
    if A == reg[B]:
        reg[C] = 1
    else:
        reg[C] = 0
def eqri(reg, A, B, C):
    if reg[A] == B:
        reg[C] = 1
    else:
        reg[C] = 0
def eqrr(reg, A, B, C):
    if reg[A] == reg[B]:
        reg[C] = 1
    else:
        reg[C] = 0

operations = (addr,addi,mulr,muli,banr,bani,borr,bori,setr,seti,gtir,gtri,gtrr,eqir,eqri,eqrr)

def test_entry(entry):
    opmatches = []
    before, instructions, after = entry
    opcode, A, B, C = instructions
    for op in operations:
       reg = before.copy()
       op(reg, A, B, C)
       if reg == after:
           opmatches.append(op)
    return (opcode, opmatches)


if __name__ == '__main__':
    state, part2 = read_input('input_16.txt')
    p1count = 0
    samples = {}
    for entry in state:
        before, ops, after = entry
        print(before, end=' => ')
        print(ops, end=' => ')
        print(after, end=': ')
        opcode, opmatches = test_entry(entry)
        if opcode not in samples:
            samples[opcode] = opmatches
        if len(opmatches) >= 3:
            p1count += 1
        print(f'[{opcode}]: {list(x.__name__ for x in opmatches)}')

    print(f'Samples with 3 or more opcodes {p1count}')

    ophash = {}
    not_done = True
    ssamples = sorted(samples.items(), key=lambda x: len(x[1]))
    while not_done:
        not_done = False
        for o,s in ssamples:
            print(f'[{o}]: {list(x.__name__ for x in s)}')
            i = 0
            while True:
                if i == len(s):
                    break
                if len(s) == 1:
                    ophash[s[0]] = o
                    del s[0]
                    not_done = True
                    break
                else:
                    if s[i] in ophash:
                        del s[i]
                        not_done = True
                    else:
                        i += 1
            print(f' => {list(x.__name__ for x in s)}')
        ssamples.sort(key=lambda x: len(x[1]))

    for k, v in sorted(ophash.items(), key=lambda x: x[1]):
        print(f'{k.__name__}: [{v}]')

    ops = {v: k for k, v in ophash.items()}
    reg = [0,0,0,0]
    for op, A, B, C in part2:
        f = ops[op]
        print(reg, end=' => ')
        print(f'{ops[op].__name__}, {A}, {B}, {C}', end=' => ')
        f(reg, A,B,C)
        print(reg)
    print(f'Register 0 has value {reg[0]}')

