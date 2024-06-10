ns    = set()
dists = set()
sorts = set()
shufs = set()
mems  = set()

with open("result.txt", "r") as f:
    lines = f.readlines()

for line in lines:
    stripped = line.strip()

    splittedPipe = stripped.split("|")
    if len(splittedPipe) == 2:
        dist = splittedPipe[0].strip()
        n    = splittedPipe[1].split(",")[0].strip().split(" ")[0].strip()

        if dist not in dists:
            dists.add(dist)

        if n not in ns:
            ns.add(n)
            
        continue

    splittedColon = stripped.split(":")
    if len(splittedColon) == 2:
        sortinfo = splittedColon[0].split("(")
        sortname = sortinfo[0].strip()
        mem      = sortinfo[1].replace(")", "").strip()

        if sortname not in sorts:
            sorts.add(sortname)

        if mem not in mems:
            mems.add(mem)

        continue

    shuf = stripped.replace(".", "")
    if shuf not in shufs:
        shufs.add(shuf)

res = {
    sort: {
        n: {
            shuf: {
                mem: {
                    dist: None
                    for dist in dists
                }
                for mem in mems
            }
            for shuf in shufs
        }
        for n in ns
    }
    for sort in sorts
}

currn = None
currd = None
currs = None

for line in lines:
    stripped = line.strip()

    splittedPipe = stripped.split("|")
    if len(splittedPipe) == 2:
        currd = splittedPipe[0].strip()
        currn = splittedPipe[1].split(",")[0].strip().split(" ")[0].strip()
        continue

    splittedColon = stripped.split(":")
    if len(splittedColon) == 2:
        time     = splittedColon[1].strip()
        sortinfo = splittedColon[0].split("(")
        sortname = sortinfo[0].strip()
        mem      = sortinfo[1].replace(")", "").strip()

        res[sortname][currn][currs][mem][currd] = time
        continue

    currs = stripped.replace(".", "")

# group by shuffle, different tables for each n and each unique amount
for n in ns:
    for dist in dists:
        with open(f"tables/shufs_{n}_{dist}.csv", "w") as f:
            f.write(f"{n}_{dist},")
            
            out = ""
            for sort in sorts:
                for mem in res[sort][n][next(iter(shufs))]:
                    if res[sort][n][shuf][mem][dist] is not None:
                        out += f"{sort} ({mem}),"

            f.write(out[:-1] + "\n")
            
            for shuf in shufs:
                f.write(shuf + ",")

                out = ""
                for sort in sorts:
                    for mem in res[sort][n][shuf]:
                        if res[sort][n][shuf][mem][dist] is not None:
                            out += res[sort][n][shuf][mem][dist] + ","

                f.write(out[:-1] + "\n")

# group by shuffle, different tables for each n, unique amount and memory
for n in ns:
    for dist in dists:
        for mem in mems:
            with open(f"tables/shufs_{n}_{dist}_{mem}.csv", "w") as f:
                f.write(f"{n}_{dist}_{mem},")
                
                out = ""
                for sort in sorts:
                    if res[sort][n][shuf][mem][dist] is not None:
                        out += f"{sort} ({mem}),"

                f.write(out[:-1] + "\n")
                
                for shuf in shufs:
                    f.write(shuf + ",")

                    out = ""
                    for sort in sorts:
                        if res[sort][n][shuf][mem][dist] is not None:
                            out += res[sort][n][shuf][mem][dist] + ","

                    f.write(out[:-1] + "\n")

# group by memory, different tables for each n, unique amount and shuffle
for n in ns:
    for dist in dists:
        for shuf in shufs:
            with open(f"tables/mems_{n}_{dist}_{shuf}.csv", "w") as f:
                f.write(f"{n}_{dist}_{shuf},")

                out = ""
                for sort in sorts:
                    out += sort + ","
                f.write(out[:-1] + "\n")

                for mem in res[sort][n][shuf]:
                    f.write(mem + ",")

                    out = ""
                    for sort in sorts:
                        if res[sort][n][shuf][mem][dist] is not None:
                            out += res[sort][n][shuf][mem][dist] + ","
                        else:
                            out += " ,"

                    f.write(out[:-1] + "\n")