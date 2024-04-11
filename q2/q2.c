#include <_regex.h>
#include <omp.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

double getElapsedTime(struct timespec start, struct timespec end) {
  double elapsedTime;
  elapsedTime =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1E9;
  return elapsedTime;
}

typedef struct DFA_Transition {
  char *regex_pattern; // The input symbol that triggers this transition
  regex_t *regex;
  int targetState; // Pointer to the target DFA node after the transition
  int match_increment;
} DFA_Transition;

typedef struct DFA_Node {
  int state;
  DFA_Transition *transitions;
  int num_transitions;
} DFA_Node;

typedef struct SegmentTraversalInfo {
  int segmentStart;
  int segmentLength;
  DFA_Node *endNodeIfStartAtDFA[4];
  int matchCountIfStartAtDFA[4];
} SegmentTraversalInfo;

DFA_Node *dfaStates[4];

char choseFrom[] = {'0', '0', '1', '1', '2', '3', '4', '5', '6', '7', '8',
                    '9', 'a', 'b', 'c', 'd', 'e', 'f', 'x', 'x', 'y'};
int choseFromLength = 20;

/*_______DEFINE DFA________*/
DFA_Transition dfa0T[] = {{.regex_pattern = "[^1-9]", .targetState = 0},
                          {.regex_pattern = "[1-9]", .targetState = 1}};
DFA_Transition dfa1T[] = {{.regex_pattern = "[1-9a-f]", .targetState = 2},
                          {.regex_pattern = "[^1-9a-f]", .targetState = 0}};
DFA_Transition dfa2T[] = {
    {.regex_pattern = "[0-9a-f]", .targetState = 3},
    {.regex_pattern = "[^0-9a-f]", .targetState = 0},
};
DFA_Transition dfa3T[] = {
    {.regex_pattern = "[0-9a-f]", .targetState = 3},
    {.regex_pattern = "[^0-9a-f]", .targetState = 0, .match_increment = 1}};

DFA_Node dfa0 = {.state = 0, .transitions = dfa0T, .num_transitions = 2};
DFA_Node dfa1 = {.state = 1, .transitions = dfa1T, .num_transitions = 2};
DFA_Node dfa2 = {.state = 2, .transitions = dfa2T, .num_transitions = 2};
DFA_Node dfa3 = {.state = 3, .transitions = dfa3T, .num_transitions = 2};
/* _______________________ */

void initDfa() {
  printf("building dfa");
  dfaStates[0] = &dfa0;
  dfaStates[1] = &dfa1;
  dfaStates[2] = &dfa2;
  dfaStates[3] = &dfa3;
  // create regex for transitions
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      regex_t *regex = (regex_t *)malloc(sizeof(regex_t));
      int ret;
      // Compile the regular expression
      const char *pattern =
          dfaStates[i]
              ->transitions[j]
              .regex_pattern; // matches a string with only letters
      ret = regcomp(regex, pattern, REG_EXTENDED);
      if (ret) {
        printf("Could not compile regex\n");
        exit(1);
      }
      dfaStates[i]->transitions[j].regex = regex;
    }
  }
  printf("done building dfa");
}

void cleanUpDFA() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      free(dfaStates[i]->transitions[j].regex);
    }
  }
}

char *genRandomStr(int stringLength) {
  char *str = (char *)malloc(stringLength * sizeof(char));
  srand(time(NULL));
  for (int i = 0; i < stringLength; i++) {
    int randomIndex = rand() % choseFromLength;
    str[i] = choseFrom[randomIndex];
  }
  return str;
}

DFA_Transition *getNextTransition(char curChar, DFA_Node *dfaNode) {
  for (int i = 0; i < dfaNode->num_transitions; i++) {
    // Execute regular expression
    int ret;
    char toMatch[2];
    toMatch[0] = curChar;
    toMatch[1] = '\0';
    ret = regexec(dfaNode->transitions[i].regex, toMatch, 0, NULL, 0);
    if (!ret) {
      return &dfaNode->transitions[i];
    } else if (ret == REG_NOMATCH) {
    } else {
      printf("Regex match failed\n");
      exit(1);
    }
  }
  printf("IMPOSSIBLE SCENARIO, DFA DIDNT MATCH\n");
  exit(-1);
}

int main(int argc, char *argv[]) {
  // initialize DFA
  initDfa();

  // Read the args
  int numOfOptimisticThreads = atoi(argv[1]);
  int stringLength = atoi(argv[2]);
  int numOfThreads = numOfOptimisticThreads + 1;

  // configure openmp
  omp_set_dynamic(0); /* Disable dynamic teams. */
  omp_set_num_threads(numOfThreads);

  // generate random string
  char *genStr = genRandomStr(stringLength + 1);

  // split into numOfThreads segments
  SegmentTraversalInfo traversalInfo[numOfThreads];
  int lengthOfSegments = stringLength / numOfThreads;
  int numOfSegmentsWithExtraChar = stringLength % numOfThreads;

  struct timespec start, end;

  // Start timer
  clock_gettime(CLOCK_MONOTONIC, &start);
  // compute parallel work
#pragma omp parallel
  {
    // determine the part of the array we will try to match on the DFA
    int threadId = omp_get_thread_num();
    if (threadId <= numOfSegmentsWithExtraChar) {
      traversalInfo[threadId].segmentStart = (lengthOfSegments + 1) * threadId;
      traversalInfo[threadId].segmentLength = lengthOfSegments + 1;
    } else {
      traversalInfo[threadId].segmentStart =
          (lengthOfSegments + 1) * numOfSegmentsWithExtraChar +
          (lengthOfSegments) * (threadId - numOfSegmentsWithExtraChar);
      traversalInfo[threadId].segmentLength = lengthOfSegments;
    }

    for (int dfaIdx = 0; dfaIdx < 4;
         dfaIdx++) { // use each state as a possible start state

      if (threadId == 0 &&
          dfaIdx != 0) // first thread will always start at start state
        continue;

      char *curChar = &genStr[traversalInfo[threadId].segmentStart];
      DFA_Node *curDfa = dfaStates[dfaIdx];
      int matches = 0;
      for (int j = 0; j < traversalInfo[threadId].segmentLength; j++) {
        DFA_Transition *nextTransition = getNextTransition(*curChar, curDfa);
        matches += nextTransition->match_increment;
        curChar++;
        curDfa = dfaStates[nextTransition->targetState];
      }
      traversalInfo[threadId].endNodeIfStartAtDFA[dfaIdx] = curDfa;
      traversalInfo[threadId].matchCountIfStartAtDFA[dfaIdx] = matches;
    }
  }

  // traverse using computed subproblems and start state
  int matches = traversalInfo[0].matchCountIfStartAtDFA[0];
  DFA_Node *curDfa = traversalInfo[0].endNodeIfStartAtDFA[0];
  for (int i = 1; i < numOfThreads; i++) {
    matches += traversalInfo[i].matchCountIfStartAtDFA[curDfa->state];
    curDfa = traversalInfo[i].endNodeIfStartAtDFA[curDfa->state];
  }
  // Record the end time
  // Stop timer
  clock_gettime(CLOCK_MONOTONIC, &end);

  // Compute the elapsed time in seconds
  double elapsedTime = getElapsedTime(start, end);

  free(genStr);
  printf("matches : %d\n", matches);

  printf("matching took %f seconds to execute \n", elapsedTime);
  cleanUpDFA();
  return 0;
}
