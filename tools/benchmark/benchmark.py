import subprocess
import argparse
import math


def run_experiment(args, experiment, result_file):
  benchmark_exec = './benchmark -p %d -s %d -np %d -ns %d -u %s -pass %s -ser %s -t %d -d %d -port %d'
  np = math.ceil(experiment[2]/experiment[0])
  ns = math.ceil(experiment[2]/experiment[1])

  error_file = open('error.log','w')
 # ssh_cmd = 'ssh %s@%s;;exit' % (args.serveruser,args.servername)
 # ssh_process = subprocess.Popen(ssh_cmd, stderr = error_file, shell=False)

  # setup ejabberd

  # begin ps logging

  # exit from ssh session
  #ssh_process.stdin(['exit'])

  bench_cmd = benchmark_exec % (experiment[0], experiment[1],np,ns,args.ejabberduser,
      args.ejabberdpass,args.servername,int(args.experimenttime), 2000, int(args.ejabberdport))
  print bench_cmd
  benchmark_process = subprocess.call(bench_cmd, stdout=result_file, stderr=error_file, shell=True)
  error_file.close()

if __name__ == "__main__":
# OPTION PARSING
  parser = argparse.ArgumentParser(description='Pubsub Ejabberd benchmark')

  parser.add_argument('--servername', metavar='-sn', dest = 'servername', type=str,
                       help='ejabberd server domain name',required=True)
  parser.add_argument('--serveruser', metavar='-su', dest='serveruser',
    help='ejabbered server user to ssh into with',required=True)
  parser.add_argument('--serverpass', metavar='-sp', dest='serverpass',
    help='ejabbered server user password to authenticate with',required=True)

  parser.add_argument('--ejabberduser', metavar='-eu', dest='ejabberduser',
    help='ejabbered user to restart server and change configuration',required=True)
  parser.add_argument('--ejabberdpass', metavar='-ep', dest='ejabberdpass',
    help='ejabbered pass to restart server and change configuration',required=True)
  parser.add_argument('--ejabberdport', metavar='-epn', dest='ejabberdport',
    help='port of ejabberd server',required=True)

  parser.add_argument('--experimentconfig', metavar='-c', dest='experimentconfig',
    help='experiment configuration file. npub,nsub,nevent',required=True)
  parser.add_argument('--experimenttime', metavar='-t', dest ='experimenttime',
    help='time in seconds for experiment to run',required=True);

  parser.add_argument('--resultpath', metavar='-rp', dest ='resultpath',
    help='Path to store result output',required=True);
  args = parser.parse_args()

  # Extract experiments
  experiment_params = []
  config_file = open(args.experimentconfig, 'r')

  line = config_file.readline()
  while line != '':
    experiment = [int(x) for x in line.split(',') if x.strip()]
    experiment_params.append(experiment)
    line = config_file.readline()
  config_file.close()
  result_file = open(args.resultpath, 'w')

  for experiment in experiment_params:
    run_experiment(args, experiment, result_file)
  result_file.close()
