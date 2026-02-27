#include <sys/time.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include "cvi_rc_kernel.h"

using namespace std;

#define PASS 1
#define FAIL 0

#define EPS (0.0001)

typedef struct _stRcPicGolden_ {
  int targetBit;
  int picQp;
  RC_Float picLambda;
  RC_Float alpha;
  RC_Float beta;
  RC_Float encodedLambda;
} stRcPicGolden;

string readCsvFileIntoString(const string &path)
{
  auto ss = ostringstream{};
  ifstream input_file(path);
  if (!input_file.is_open()){
    cerr << "Could not open the file - '" << path << "'" << endl;
    exit(EXIT_FAILURE);
  }
  ss << input_file.rdbuf();
  return ss.str();
}

int analyzeCsvString(string file_contents, std::map<int, std::vector<string>> &csv_contents)
{
  istringstream sstream(file_contents);
  std::vector<string> items;
  string record;
  int line_cnt = 0;
  while (std::getline(sstream, record))
  {
    istringstream line(record);
    while (std::getline(line, record, ',')) {
      items.push_back(record);
    }
    csv_contents[line_cnt] = items;
    items.clear();
    line_cnt++;
  }
  return line_cnt;
}

double rcFloat_to_double(RC_Float v)
{
#if SOFT_FLOAT
  float* dp = (float*)&v;
  return (double)(*dp);
#else
  return v;
#endif
}

RC_Float double_to_rcFloat(float v)
{
#if SOFT_FLOAT
  float fv = (float)v;
  RC_Float* dp = (RC_Float*)&fv;
  return (RC_Float)(*dp);
#else
  return (RC_Float)v;
#endif
}

void parse_rcKernelCfg(stRcKernelCfg* cfg, std::vector<string> cfg_fields, int &frameNum)
{
  cfg->codec = std::stoi(cfg_fields[0]);
  frameNum = std::stoi(cfg_fields[1]);
  cfg->targetBitrate = std::stoi(cfg_fields[2]);
  cfg->framerate = double_to_rcFloat((float)stoi(cfg_fields[3]));
  cfg->intraPeriod = std::stoi(cfg_fields[4]);
  cfg->statTime = std::stoi(cfg_fields[5]);
  cfg->ipQpDelta = std::stoi(cfg_fields[6]);
  cfg->numOfPixel = std::stoi(cfg_fields[7]);
	cfg->maxIprop = std::stoi(cfg_fields[8]);
  cfg->minIprop = std::stoi(cfg_fields[9]);
  cfg->maxQp = std::stoi(cfg_fields[10]);
  cfg->minQp = std::stoi(cfg_fields[11]);
  cfg->maxIQp = std::stoi(cfg_fields[12]);
  cfg->minIQp = std::stoi(cfg_fields[13]);
  cfg->firstFrmstartQp = std::stoi(cfg_fields[14]);
  cfg->rcMdlUpdatType = std::stoi(cfg_fields[15]);
}

void parse_encPictureStats(stRcPicGolden* picGolden, stRcKernelPicIn *stats, std::vector<string> cfg_fields)
{
  picGolden->targetBit = ::stoi(cfg_fields[0]);
	picGolden->picQp = ::stoi(cfg_fields[1]);
	picGolden->picLambda = double_to_rcFloat(stof(cfg_fields[2]));
  stats->encodedBit = ::stoi(cfg_fields[3]);
	stats->encodedQp = double_to_rcFloat(stof(cfg_fields[4]));
  picGolden->encodedLambda = double_to_rcFloat(stof(cfg_fields[5]));
	stats->madi = double_to_rcFloat(stof(cfg_fields[6]));
  stats->mse = double_to_rcFloat(stof(cfg_fields[7]));
  stats->skipRatio = double_to_rcFloat(stof(cfg_fields[8]));
  picGolden->alpha = double_to_rcFloat(stof(cfg_fields[9]));
  picGolden->beta = double_to_rcFloat(stof(cfg_fields[10]));
  stats->encodedLambda = double_to_rcFloat(0);
}

int compare_rc_est(stRcPicGolden* picGolden, stRcKernelPicOut *rcPicOut, stRcKernelInfo *info)
{
  if((abs(picGolden->targetBit - rcPicOut->targetBit)/(double)info->numOfPixel) > EPS) {
    printf("targetBit mismatch! golden, %d test, %d\n", picGolden->targetBit, rcPicOut->targetBit);
    return FAIL;
  }
  if(picGolden->picQp != rcPicOut->qp) {
    printf("picQp mismatch! golden, %d test, %d\n", picGolden->picQp, rcPicOut->qp);
    return FAIL;
  }

  double picLambda = rcFloat_to_double(picGolden->picLambda);
  double lambda = rcFloat_to_double(rcPicOut->lambda);
  if((abs(picLambda - picLambda)/picLambda) > EPS) {
    printf("picLambda mismatch! golden, %.4f test,%.4f\n", picLambda, lambda);
    return FAIL;
  }
  // synchronize floating-point param to prevent error propagation
  info->picTargetBit = picGolden->targetBit;
  return PASS;
}

int compare_rc_update(stRcPicGolden* picGolden, stRcKernelPicOut *rcPicOut, stRcKernelInfo *info, int isIPic)
{
  double encodedLambda = rcFloat_to_double(picGolden->encodedLambda);
  double lastPicLambda = rcFloat_to_double(info->lastPicLambda);
  if((abs(encodedLambda - lastPicLambda)/picGolden->encodedLambda) > EPS) {
    printf("codedLambda mismatch! golden, %.4f test,%.4f\n", encodedLambda, lastPicLambda);
    return FAIL;
  }
  int picLv = isIPic==0;
  double golden_alpha = rcFloat_to_double(picGolden->alpha);
  double alpha = rcFloat_to_double(info->rqModel[picLv].alpha);
  double golden_beta = rcFloat_to_double(picGolden->beta);
  double beta = rcFloat_to_double(info->rqModel[picLv].beta);
  if((abs(golden_alpha - alpha)/golden_alpha) > EPS) {
    printf("alpha mismatch! golden, %.4f test,%.4f\n", golden_alpha, alpha);
    return FAIL;
  }
  if((abs(golden_beta - beta)/golden_beta) > EPS) {
    printf("beta mismatch! golden, %.4f test,%.4f\n", golden_beta, beta);
    return FAIL;
  }
  // synchronize floating-point param to prevent error propagation 
  info->lastPicLambda = picGolden->encodedLambda;
  info->lastLevelLambda[picLv] = picGolden->encodedLambda;
  info->rqModel[picLv].alpha = picGolden->alpha;
  info->rqModel[picLv].beta = picGolden->beta;
  return PASS;
}

void parse_command(int argc, char *argv[], string &file_name, string &base_dir)
{
  string file_key = "-f";
  string dir_key = "-dir";
  for (int arg_i = 0; arg_i < argc; arg_i++)
  {
    string key = argv[arg_i];
    if (!file_key.compare(key)) {
      file_name = argv[arg_i + 1];
    }
    else if (!dir_key.compare(key)) {
      base_dir = argv[arg_i + 1];
    }
  }
}

int main(int argc, char *argv[])
{
  string base_dir, file_name;
  parse_command(argc, argv, file_name, base_dir);

  string file_dir = base_dir + "/" + file_name;
  string file_contents = readCsvFileIntoString(file_dir);
  std::map<int, std::vector<string>> csv_cmds;
  int csv_line = analyzeCsvString(file_contents, csv_cmds);
  stRcKernelCfg cfg;
  stRcPicGolden picGolden;
  stRcKernelPicIn picIn;
  stRcKernelPicOut picOut;
  stRcKernelInfo info;
  int frameNum = 0;
  parse_rcKernelCfg(&cfg, csv_cmds[1], frameNum);
  frameNum = std::min(frameNum, (int)csv_cmds.size()-3);
  struct timeval startTime, endTime;
  int64_t estRcTimeAccum = 0, updateRcTimeAccum = 0;
  ios_base::sync_with_stdio(false);  // unsync the I/O of C and C++.
  cviRcKernel_init(&info, &cfg);
  for (int frame_i = 0; frame_i < frameNum; frame_i++)
  {
    parse_encPictureStats(&picGolden, &picIn, csv_cmds[frame_i + 3]);

    int isIPic = (frame_i % cfg.intraPeriod) == 0;
    // start timer.
    gettimeofday(&startTime, NULL);

    cviRcKernel_estimatePic(&info, &picOut, isIPic, frame_i);

    // stop timer.
    gettimeofday(&endTime, NULL);
    estRcTimeAccum += ((endTime.tv_sec - startTime.tv_sec) * 1e6 + (endTime.tv_usec - startTime.tv_usec));

    if(compare_rc_est(&picGolden, &picOut, &info)==FAIL) {
      printf("compare rc_est fail at frame %d\n", frame_i);
      break;
    }
    // start timer.
    gettimeofday(&startTime, NULL);

    cviRcKernel_updatePic(&info, &picIn, isIPic);

    // stop timer.
    gettimeofday(&endTime, NULL);
    updateRcTimeAccum += ((endTime.tv_sec - startTime.tv_sec) * 1e6 + (endTime.tv_usec - startTime.tv_usec));

    if(compare_rc_update(&picGolden, &picOut, &info, isIPic)==FAIL) {
      printf("compare rc_update fail at frame %d\n", frame_i);
      break;
    }
    printf("pass frame %d\n", frame_i);
  }
  printf("exe time(us), %.3f,  %.3f, %.3f\n",  \
  estRcTimeAccum/(double)frameNum, \
  updateRcTimeAccum/(double)frameNum, \
  (estRcTimeAccum+updateRcTimeAccum)/(double)frameNum);
  return 0;
}
