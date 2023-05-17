filePath = "/mnt/c/Users/dell/Documents/Centrale-Nantes/EI4-INFOIA/TFE/TAPAAL/tapn_examples/time/"
fileNames = ["MCC22-2cee3b71.7cca7fe1.RDFS.ReachabilityCardinality.txt",
             "MCC22-2cee3b71.7cca7fe1.RDFS.ReachabilityFireability.txt"]
for file in fileNames:
    with open(filePath + file, "r") as f:
        lines = f.readlines()
        nbSup1Sec = 0
        nbSup10Sec = 0
        sum = 0.
        for line in lines:
            sum += float(line)
            if float(line) > 10:
                nbSup10Sec += 1
            if float(line) > 1:
                nbSup1Sec += 1
        print(f"{file} : avg={sum/len(lines)}, {nbSup1Sec=} ({100*nbSup1Sec/len(lines):2.1f}%), {nbSup10Sec=} ({100*nbSup10Sec/len(lines):2.1f}%) on {len(lines)} in total")