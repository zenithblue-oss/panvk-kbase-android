import re, sys
# Print unique disassembled opcodes (between the last "clause_0:" before each HC END and HC END).
for path in sys.argv[1:]:
    txt = open(path).read().split("\n")
    ops = {}
    in_dis = False
    for l in txt:
        if l.startswith("clause_0:"):
            in_dis = True
            ops = {}
        if l.startswith("=== HC END"):
            print(path, l, " ".join(sorted(ops)))
            in_dis = False
        if in_dis:
            m = re.match(r"\s+([*+])(\S+)", l)
            if m:
                ops[m.group(1) + m.group(2)] = 1
