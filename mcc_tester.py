from argparse import ArgumentParser
from enum import Enum
from pathlib import Path
from typing import List
import time
import asyncio

parser = ArgumentParser(prog="Petri net query tester")
parser.add_argument('-m', '--models', help="Path to directory containing the mcc models", default='/usr/local/share/mcc/')
parser.add_argument('-t', '--timeout', help="Timeout for each query of a model in seconds", type=int, default=2)
parser.add_argument('-b', '--binary', help="Path to verifypn", default='build-release/verifypn/bin/verifypn-linux64')
parser.add_argument('-o', '--out', help="Name of csv file containing the results", default='results.csv')
parser.add_argument('-n', '--threads', help="Amount of threads started", type=int, default=1)
parser.add_argument('-w', '--worker-count', help="The amount of workers", type=int, default=1)
parser.add_argument('-i', '--worker-index', help="Index of the worker", type=int, default=0)
parser.add_argument('-e', '--error-directory', help="Index of the worker", type=int, default=0)

args = parser.parse_args()

MODELS_PATH = args.models
QUERY_TIMEOUT = args.timeout
VERIFYPN_PATH = args.binary
OUT_PATH = args.out
THREADS = args.threads
WORKER_COUNT = args.worker_count
WORKER_INDEX = args.worker_index
ERROR_DIRECTORY = args.error_directory

QUERY_IS_SATISFIED = "Query is satisfied"
QUERY_IS_NOT_SATISFIED = "Query is NOT satisfied"

class QueryResult(Enum):
    SATISFIED = "satisfied"
    UNSATISFIED = "unsatisfied"
    TIMEOUT = "timeout"
    ERROR = "error"
    

class ResultWriter:
    def __init__(self, resultFilePath):
        self.file = open(resultFilePath, "w")
        self.file.write("model name,query name,query index,result,time\n")
    
    def addResult(self, modelName: str, queryName: str, queryIndex: int, queryResult: QueryResult, realTime: float):
        self.file.write(f"{modelName},{queryName},{queryIndex},{queryResult.value},{realTime}\n") 

    def flush(self):
        self.file.flush()

class RunMetrics:
    def __init__(self, realTime: float, result: QueryResult):
        self.realTime = realTime
        self.result = result


class QueryFile:
    def __init__(self, queryPath):
        self.queryPath = queryPath
        with open(queryPath, 'r') as content_file:
            self.queryCount = content_file.read().count("<property>")
    def name(self):
        return self.queryPath.name

class Model:
    def __init__(self, modelRoot, modelPath, queryFiles):
        self.modelPath = modelPath
        self.queryFiles = queryFiles
        self.modelRoot = modelRoot
    def __repr__(self):
        return self.name()
    def name(self):
        return self.modelRoot.name

class ModelCheckingJob:
    def __init__(self, model: Model, queryFile: QueryFile, queryIndex: int):
        self.model = model
        self.queryFile = queryFile
        self.queryIndex = queryIndex

    def __repr__(self):
        return f"{self.model.name()} {self.queryFile.name()}:{self.queryIndex}"

    def getCommand(self):
        command = ["-C", "-x", str(self.queryIndex)]
        if ("LTL" in str(self.queryFile.queryPath)):
            command.extend(["-ltl", "tarjan"])
        command.extend([str(self.model.modelPath), str(self.queryFile.queryPath)])
        return command

    async def run(self) -> RunMetrics:
        command = self.getCommand()
        startTime = time.time()
        proc = await asyncio.create_subprocess_exec(VERIFYPN_PATH, *command, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
        try:
            bout, berr = await asyncio.wait_for(proc.communicate(), timeout=QUERY_TIMEOUT)
            out = bout.decode("utf-8")
            err = berr.decode("utf-8")
            if (proc.returncode != 0):
                print("Error")
                print(err)
                return RunMetrics(time.time() - startTime, QueryResult.ERROR)
            else:
                return RunMetrics(time.time() - startTime, extractResultFromOutput(out))
        except asyncio.TimeoutError:
            print("timeout")
            proc.terminate()
            return RunMetrics(QUERY_TIMEOUT, QueryResult.TIMEOUT)


models: List[Model] = []

for modelRoot in Path(MODELS_PATH).iterdir():
    modelPnml = modelRoot / "model.pnml"
    queryFiles = [QueryFile(x) for x in modelRoot.glob('ReachabilityCardinality.xml') if "GenericProperties" not in x.name]
    models.append(Model(modelRoot, modelPnml, queryFiles))

print(f"Found {models.__len__()} models")
print(f"Starting with {QUERY_TIMEOUT} second timeout")

def extractResultFromOutput(out):
    if (QUERY_IS_SATISFIED in out):
        return QueryResult.SATISFIED
    elif (QUERY_IS_NOT_SATISFIED in out):
        return QueryResult.UNSATISFIED
    else:
        return QueryResult.ERROR

modelCheckingJobs: List[ModelCheckingJob] = []

for model in models:
    for queryFile in model.queryFiles:
        for queryIndex in range(1, queryFile.queryCount + 1):
            modelCheckingJobs.append(ModelCheckingJob(model, queryFile, queryIndex))

modelCheckingJobs.sort(key=lambda mcj : repr(mcj))

resultWriter = ResultWriter(OUT_PATH)

print(f"Running {len(modelCheckingJobs)} jobs on {THREADS} threads")

nextjob = WORKER_INDEX

async def async_main():
    await asyncio.gather(*[async_thread(i) for i in range(0, THREADS)])

async def async_thread(threadId: int):
    global nextjob 
    while True:
        jobindex = nextjob
        nextjob += WORKER_COUNT
        if nextjob >= len(modelCheckingJobs):
            return
        job = modelCheckingJobs[jobindex]
        total_jobs = int(len(modelCheckingJobs)/WORKER_COUNT)
        current_job = int(jobindex/WORKER_COUNT)
        print(f"{current_job}/{total_jobs}\t WC {int(((total_jobs-current_job) * QUERY_TIMEOUT)/THREADS)} seconds\t{threadId} # {job}")
        result = await job.run()
        resultWriter.addResult(job.model.name(), job.queryFile.name(), job.queryIndex, result.result, result.realTime)

try:
    asyncio.run(async_main())
except KeyboardInterrupt:
    ResultWriter.flush()


