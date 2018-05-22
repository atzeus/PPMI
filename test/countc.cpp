#include <unordered_map>
#include <iostream>
#include <fstream>
#include <cstring>
#include <functional>
#include <sstream>
#include <vector>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <algorithm>    // sort
#include <random>
#include <stdlib.h>     /* srand, rand */
#include "mmio.h"
#include "../src/redsvd.hpp"

using namespace std;

bool myfunction (pair<int,unordered_map<int, int>> i,pair<int,unordered_map<int, int>> j) { return (i.first < j.first); }
bool myfunction2 (pair<int,int> i,pair<int,int> j) { return (i.second > j.second); }
#define NRFILE_ARGS 4

bool tryGetBoolArg(int argc, char* argv[],const char* arg){
    for(int i = 1 ; i < argc - NRFILE_ARGS; i++){
        if(strcmp(argv[i],arg) == 0 && i + 1 < argc - NRFILE_ARGS){
           return true;
        }
    }
    return false;
}



double tryGetFloatArg(int argc, char* argv[],const char* arg, double defaults){
    for(int i = 1 ; i < argc - NRFILE_ARGS; i++){
        printf("%s\n", argv[i]);
        if(strcmp(argv[i],arg) == 0 && i + 1 < argc - NRFILE_ARGS){
            printf("got %s\n", arg);
            double res;
            if(sscanf(argv[i+1], "%lf",&res) == 1){
                cout << res << " gotten\n";
                return res;
            } else {
                cout << "Could not parse float \n";
            }
        }
    }
    return defaults;
}


int tryGetNumArg(int argc, char* argv[],const char* arg, int defaults){
    for(int i = 1 ; i < argc - NRFILE_ARGS; i++){
        if(strcmp(argv[i],arg) == 0 && i + 1 < argc - NRFILE_ARGS){
            int res;
            if(sscanf(argv[i+1], "%d",&res) == 1){
                return res;
            } 
        }
    }
    return defaults;
}



#define CORPUS_INPUT 1
#define VOCAB_INPUT 2
#define PAIRS_INPUT 3
#define PPMI_INPUT 4
#define SVD_INPUT 5

int main(int argc, char *argv[])
{
    clock_t t;
    if(argc < NRFILE_ARGS){   
        cerr << "Usage: " << argv[0] << " corpus.txt vocabulary pairsfile ppmi.mm svd" << endl; 
    }
    int thr = tryGetNumArg(argc,argv,"--thr",100);
    int rank = tryGetNumArg(argc,argv,"--rank",500);
    int win = tryGetNumArg(argc,argv,"--win",2); 
    double subsample = tryGetFloatArg(argc,argv,"--sub",0);
    double cds = tryGetFloatArg(argc,argv,"--cds",0.75f);
    bool pos = tryGetBoolArg(argc,argv,"--pos");
    bool dyn = tryGetBoolArg(argc,argv,"--dyn");
    bool del = tryGetBoolArg(argc,argv,"--del");   

    cout << cds << "CDS\n";
    ifstream ifile(argv[CORPUS_INPUT]);
    if(ifile.fail()){
        cerr << "Could not input open file." << endl;
        return 1;
    }
    // produce counts per word
    int num_words = 0;
 t = clock();
    unordered_map<string, int> m;
    for (string line, word; getline(ifile, line); )
    {
        istringstream iss(line);

        while (iss >> word)
        {
           unordered_map<string, int>::iterator i = m.find(word);
           if(i == m.end()){
             m[word] = 1;
           } else {
             i->second = i->second + 1;
           }
           num_words++;
        }
    }

    ifile.close();
     t = clock() - t;
  printf ("Counting took (%f seconds).\n",((float)t)/CLOCKS_PER_SEC);
 t = clock();
    cerr << "Counted " << num_words << "\n";

    default_random_engine generator;
    uniform_real_distribution<double> distribution(0.0,1.0);

    vector<string> vocab;

    // remove deleted words
    subsample*= num_words;
   
        
    for (unordered_map<string, int>::iterator it=m.begin(); it!=m.end(); ++it){
        int count = it->second;
        if(count < thr){
            it->second = -1;
        } else if ( count > subsample) {
            float p = 1 - sqrt( subsample / count);
            double r = distribution(generator);
            if(r > p) {
                it->second = -1;
            }
        }   
        if(it->second >= 1){
            vocab.push_back(it->first);
        }

    }

    unordered_map<string, int> vocab_index;
    sort(vocab.begin(), vocab.end());

    ofstream vocabfile(argv[VOCAB_INPUT]);
    if(vocabfile.fail()){
        cerr << "Could not open file." << endl;
        return 1;
    }
    int i = 0 ;
    for(vector<string>::iterator it=vocab.begin(); it!=vocab.end(); ++it){
        vocab_index[*it] = i;
        i++;
        vocabfile << *it << '\n';
    }
    vocabfile.close();
     t = clock() - t;
  printf ("Removing took (%f seconds).\n",((float)t)/CLOCKS_PER_SEC);
  t = clock();
    cerr << "Removed!\n";

    // produce pairs
    ifstream ifile2(argv[CORPUS_INPUT]);
    if(ifile2.fail()){
        cerr << "Could not open file." << endl;
        return 1;
    }
    ofstream pairsfile(argv[PAIRS_INPUT]);
    if(pairsfile.fail()){
        cerr << "Could not open file." << endl;
        return 1;
    }
    // produce pairs
    unordered_map<int,unordered_map<int, int>> pairs;
    unordered_map<int, int> totSelf;
    unordered_map<int, double> totContext;
    int nonZero = 0;
    int nrPairs = 0;
    for (string line, word; getline(ifile2, line); ) {
        vector<string> words;
        istringstream iss(line);


        while (iss >> word) {
            if(m[word] >= 1) {
                words.push_back(word);
            }
        }
        
        for(int i = 0 ; i < words.size(); i++){
            int winsize = win;
            if(dyn) {
                winsize = (rand() % win) + 1;
            }
            int start = max<int>(0, i - winsize);
            int end = min<int> (words.size(), i + winsize + 1 );
            for(int j = start; j < end ; j++){
                if(j == i) continue;
                string p = words[i] + " " + words[j];
                pairsfile << p << '\n';
                int x = vocab_index[words[i]];
                auto px = totSelf.find(x);
                if( px == totSelf.end()){
                    totSelf[x] = 1;   
                } else {
                    px->second++;
                }

                int y = vocab_index[words[j]];
                auto py = totContext.find(x);
                if( py == totContext.end()){
                    totContext[x] = 1.0;   
                } else {
                    py->second+=1;
                }
                /*if(x == y){
                    cout << p << endl;
                    cout << line << endl;
                    cout << i << " " << j << endl;
                }*/
                
                auto pi = pairs.find(x);
                                
                if( pi == pairs.end()){
                  pairs[x] = {{y,1}}; 
                    nonZero++;
    //               cout << x << " bla " << y << " : 1" << endl;
                } else {
                   unordered_map<int, int>* m = &(pi->second);
                   
                   auto pj = m->find(y);
                   if(pj == m->end()){
                      (*m)[y] = 1;
                    nonZero++;
//                      cout << x << " jada" << y << " : 1" << endl;
                   } else {
                     pj->second = pj->second + 1;
  //                   cout << x << " " << y << " : " << pj->second <<  endl;
                   }
                  
                }
                nrPairs++;
               
            }
        }
    }
     t = clock() - t;
  printf ("Pairs took (%f seconds).\n",((float)t)/CLOCKS_PER_SEC);
  t = clock();
    double totalC = 0;

    for (auto it=totContext.begin(); it!=totContext.end(); ++it){
        it->second = pow(it->second, cds);
        totalC+= it->second;    
    }
    
     t = clock() - t;
  printf ("Counting totals took (%f seconds).\n",((float)t)/CLOCKS_PER_SEC);
  t = clock();

    MM_typecode matcode;               
    mm_initialize_typecode(&matcode);
    mm_set_matrix(&matcode);
    mm_set_coordinate(&matcode);
    mm_set_real(&matcode);

    FILE* fp = fopen(argv[PPMI_INPUT],"w+");
    mm_write_banner(fp,matcode);
    mm_write_mtx_crd_size(fp, vocab.size(), vocab.size(), nonZero);


    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    for(auto itp = pairs.begin(); itp != pairs.end(); itp++){
        for(auto itw = itp->second.begin() ; itw != itp->second.end(); itw++){
            int word = itp->first;
            int context = itw->first;
            int count = itw->second;
            double res = (count / (totSelf[word] * totContext[context])) * totalC ;
            fprintf(fp, "%d %d %lf\n" , itp->first + 1, itw->first + 1, res);
          // fprintf(fp, "%d %d %10.3g\n", itp->first+1, itw->first+1, res);
            tripletList.push_back(T(itp->first,itw->first, res ));
             //cout << "\t" << word << " " << context << " " <<  res << "\n";
        }
    }
    fclose(fp);
     t = clock() - t;
  printf ("Writing totals took (%f seconds).\n",((float)t)/CLOCKS_PER_SEC);
  t = clock();

    Eigen::SparseMatrix<float, Eigen::RowMajor> ppmi(vocab.size(), vocab.size());
    ppmi.setFromTriplets(tripletList.begin(), tripletList.end());
    REDSVD::RedSVD svd(ppmi, rank);
     t = clock() - t;
  printf ("SDV totals took (%f seconds).\n",((float)t)/CLOCKS_PER_SEC);
  t = clock();
    std::string base(argv[SVD_INPUT]);
    ofstream uout(base + ".ut.txt");
 
    for(int i = 0 ; i < svd.matrixU().rows() ; i++){
        for(int j = 0 ; j < svd.matrixU().cols() ; j++){
            uout << svd.matrixU()(i,j) << ' ';
        }
        uout << endl;
    }

 ofstream vout(base + ".vt.txt");
 
    for(int i = 0 ; i < svd.matrixV().rows() ; i++){
        for(int j = 0 ; j < svd.matrixV().cols() ; j++){
            vout << svd.matrixV()(i,j) << ' ';
        }
        vout << endl;
    }
    t = clock() - t;

   ofstream sout(base + ".s.txt");
    for(int i = 0 ; i < rank ; i++){
        sout << svd.singularValues()(i) << endl;
    }
  printf ("Writing svd took (%f seconds).\n",((float)t)/CLOCKS_PER_SEC);


  
//    cout << getSec() - start << "\t" << endl;  
  
// mat is ready to go!

        
/*
 for (int i = 0; i < row; ++i){
    A.startVec(i);
    set<int> v;
    for (;(int)v.size() < colValNum;){
      int k = rand() % col;
      if (v.find(k) != v.end()) continue;
      v.insert(k);
    }
    for (set<int>::iterator it = v.begin();
	 it != v.end(); ++it){
      A.insertBack(i, *it) = (float)rand();
    }
  }
  A.finalize();

    cerr << "Counted pairs!"; 
    Eigen::SparseMatrix<float, Eigen::RowMajor> ppmi(vocab.size(), vocab.size());

    vector<pair<int,unordered_map<int, int>>> v(pairs.begin(), pairs.end());
    sort (v.begin(), v.end(), myfunction);
    
    for (vector<pair<int, unordered_map<int, int>>>::iterator it=v.begin(); it!=v.end(); ++it){
        ppmi.startVec(it->first);
        cout << vocab[it->first] << " (" << it->first << "):\n";
        
        vector<pair<int,int>> w(it->second.begin(), it->second.end());
        sort (w.begin(), w.end(), myfunction2);
        for (vector<pair<int, int>>::iterator jt=w.begin(); jt!=w.end(); ++jt){
            A.insertBack(i, *it) = (float)rand();
            cout << "\t" << vocab[jt->first] << " (" << jt->first << "): " << jt->second << "\n";
        }
        
    }
*/

}

