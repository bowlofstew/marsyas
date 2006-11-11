#include <cstdio>

#include "MarSystemManager.h"
#include "AudioSink.h"
#include "SoundFileSink.h"
#include "SoundFileSource.h"
#include "Gain.h"
#include "Messager.h"
#include "Conversions.h"
#include "CommandLineOptions.h"
#include "PeClusters.h"
#include "PeUtilities.h"

#include <string>

using namespace std;
using namespace Marsyas;

#define EMPTYSTRING "MARSYAS_EMPTY"
string pluginName = EMPTYSTRING;
string inputDirectoryName = EMPTYSTRING;
string outputDirectoryName = EMPTYSTRING;
string fileName = EMPTYSTRING;
string noiseName = EMPTYSTRING;
string fileResName = EMPTYSTRING;
string filePeakName = EMPTYSTRING;

// Global variables for command-line options 
bool helpopt_ = 0;
bool usageopt_ =0;
int fftSize_ = 2048;
int winSize_ = 2048;
// if kept the same no time expansion
int hopSize_ = 360;
// nb Sines
int nbSines_ = 80;
// nbClusters
int nbClusters_ = 5;
// output buffer Size
int bopt = 128;
// output gain
mrs_real gopt_ = 1.0;
// number of accumulated frames
mrs_natural accSize_ = 6;
// type of similarity Metrics
string similarityType_ = "fn";
// store for clustered peaks
realvec peakSet_;
// delay for noise insertion
mrs_real noiseDelay_=44100;


bool microphone_ = false;
bool analyse_ = false;
bool attributes_ = false;
bool ground_ = false;
bool synthetize_ = false;
bool clusterSynthetize_ = false;

CommandLineOptions cmd_options;

void 
printUsage(string progName)
{
	MRSDIAG("peakClustering.cpp - printUsage");
	cerr << "Usage : " << progName << " [file]" << endl;
	cerr << endl;
	cerr << "If no filename is given the default live audio input is used. " << endl;
}

void 
printHelp(string progName)
{
	MRSDIAG("peakClustering.cpp - printHelp");
	cerr << "peakClustering, MARSYAS, Copyright Mathieu Lagrange " << endl;
	cerr << "--------------------------------------------" << endl;
	cerr << "Usage : " << progName << " [file]" << endl;
	cerr << endl;
	cerr << "if no filename is given the default live audio input is used. " << endl;
	cerr << "Options:" << endl;
	cerr << "-n --fftsize         : size of fft " << endl;
	cerr << "-w --winsize         : size of window " << endl;
	cerr << "-s --sinusoids       : number of sinusoids" << endl;
	cerr << "-b --buffersize      : audio buffer size" << endl;
	cerr << "-g --gain            : gain (0.0-1.0) " << endl;
	cerr << "-f --filename        : output filename" << endl;
	cerr << "-o --outputdirectorypath   : output directory path" << endl;
	cerr << "-i --inputdirectorypath   : input directory path" << endl;
	cerr << "-u --usage           : display short usage info" << endl;
	cerr << "-h --help            : display this information " << endl;

	exit(1);
}


// original monophonic peakClustering 


void synthNetCreate(MarSystemManager *mng, string outsfname)
{
	//create Shredder series
	MarSystem* postNet = mng->create("Series", "postNet");
	postNet->addMarSystem(mng->create("PeOverlapadd", "ob"));
	postNet->addMarSystem(mng->create("ShiftOutput", "so"));

	MarSystem *dest;
	if (outsfname == EMPTYSTRING) 
		dest = new AudioSink("dest");
	else
	{
		dest = new SoundFileSink("dest");
		//dest->updctrl("mrs_string/filename", outsfname);
	}
	MarSystem* fanout = mng->create("Fanout", "fano");
	fanout->addMarSystem(dest);
	MarSystem* fanSeries = mng->create("Series", "fanSeries");
	if (microphone_) 
		fanSeries->addMarSystem(mng->create("AudioSource", "src2"));
	else 
		fanSeries->addMarSystem(mng->create("SoundFileSource", "src2"));
	fanSeries->addMarSystem(mng->create("Delay", "delay"));
	fanout->addMarSystem(fanSeries);

	postNet->addMarSystem(fanout);
	postNet->addMarSystem(mng->create("PeResidual", "res"));

	MarSystem *destRes;
	if (outsfname == EMPTYSTRING) 
		destRes = new AudioSink("destRes");
	else
	{
		destRes = new SoundFileSink("destRes");
		//dest->updctrl("mrs_string/filename", outsfname);
	}

	MarSystem* shredNet = mng->create("Shredder", "shredNet");
	shredNet->addMarSystem(postNet);

	mng->registerPrototype("PeSynthetize", shredNet);
}

void
synthNetConfigure(MarSystem *pvseries, string sfName, string outsfname, mrs_natural Nw, 
									mrs_natural D, mrs_natural S, mrs_natural accSize)
{
	pvseries->updctrl("Shredder/synthNet/mrs_natural/nTimes", accSize);
	pvseries->updctrl("Shredder/synthNet/Series/postNet/PeOverlapadd/ob/mrs_natural/hopSize", D);
	pvseries->updctrl("Shredder/synthNet/Series/postNet/PeOverlapadd/ob/mrs_natural/nbSinusoids", S);
	pvseries->updctrl("Shredder/synthNet/Series/postNet/PeOverlapadd/ob/mrs_natural/delay", Nw/2+1);
	pvseries->updctrl("Shredder/synthNet/Series/postNet/ShiftOutput/so/mrs_natural/Interpolation", D);
	pvseries->updctrl("Shredder/synthNet/Series/postNet/ShiftOutput/so/mrs_natural/WindowSize", Nw);      
	pvseries->updctrl("Shredder/synthNet/Series/postNet/ShiftOutput/so/mrs_natural/Decimation", D);

	if (microphone_) 
	{
		pvseries->updctrl("Shredder/synthNet/Series/postNet/Fanout/fano/Series/fanSeries/AudioSource/src2/mrs_natural/inSamples", D);
		pvseries->updctrl("Shredder/synthNet/Series/postNet/Fanout/fano/Series/fanSeries/AudioSource/src2/mrs_natural/inObservations", 1);
	}
	else
	{
		pvseries->updctrl("Shredder/synthNet/Series/postNet/Fanout/fano/Series/fanSeries/SoundFileSource/src2/mrs_string/filename", sfName);
		pvseries->updctrl("Shredder/synthNet/Series/postNet/Fanout/fano/Series/fanSeries/SoundFileSource/src2/mrs_natural/inSamples", D);
		pvseries->updctrl("Shredder/synthNet/Series/postNet/Fanout/fano/Series/fanSeries/SoundFileSource/src2/mrs_natural/inObservations", 1);
	}
	if (outsfname == EMPTYSTRING) 
		pvseries->updctrl("Shredder/synthNet/Series/postNet/AudioSink/dest/mrs_natural/bufferSize", bopt);


	pvseries->updctrl("Shredder/synthNet/Series/postNet/Fanout/fano/Series/fanSeries/Delay/delay/mrs_natural/delay", Nw+1-D);
	pvseries->updctrl("Shredder/synthNet/Series/postNet/Fanout/fano/SoundFileSink/dest/mrs_string/filename", outsfname);//[!]
	pvseries->updctrl("Shredder/synthNet/Series/postNet/SoundFileSink/destRes/mrs_string/filename", fileResName);//[!]
}

void
clusterExtract(realvec &peakSet, string sfName, string outsfname, string noiseName, mrs_real noiseDelay, string T, mrs_natural N, mrs_natural Nw, 
							 mrs_natural D, mrs_natural S, mrs_natural C,
							 mrs_natural accSize, bool synthetize)
{
	cout << "Extracting Peaks and Clusters" << endl;
	MarSystemManager mng;

	// create the phasevocoder network
	MarSystem* pvseries = mng.create("Series", "pvseries");

	//create accumulator series
	MarSystem* preNet = mng.create("Series", "preNet");
	//create fanout for mixing
	MarSystem* fanin = mng.create("Fanin", "fanin");
	// add original source in the fanout
	if (microphone_) 
		fanin->addMarSystem(mng.create("AudioSource", "src"));
	else 
		fanin->addMarSystem(mng.create("SoundFileSource", "src"));
	// create a series for the noiseSource
	MarSystem* mixseries = mng.create("Series", "mixseries");
	mixseries->addMarSystem(mng.create("SoundFileSource", "noise"));
	mixseries->addMarSystem(mng.create("Delay", "noiseDelay"));
	mixseries->addMarSystem(mng.create("Gain", "noiseGain"));
	// add this series in the fanout
	fanin->addMarSystem(mixseries);

	preNet->addMarSystem(fanin);


	preNet->addMarSystem(mng.create("ShiftInput", "si"));
	preNet->addMarSystem(mng.create("Shifter", "sh"));
	preNet->addMarSystem(mng.create("Windowing", "wi"));

	MarSystem *parallel = mng.create("Parallel", "par");
	parallel->addMarSystem(mng.create("Spectrum", "spk1"));
	parallel->addMarSystem(mng.create("Spectrum", "spk2"));
	preNet->addMarSystem(parallel);

	preNet->addMarSystem(mng.create("PeConvert", "conv"));
	//create accumulator
	MarSystem* accumNet = mng.create("Accumulator", "accumNet");
	accumNet->addMarSystem(preNet);

	/*************************************************************/

	MarSystem* peClust = mng.create("PeClust", "peClust");

	/*************************************************************/

	//create the main network
	pvseries->addMarSystem(accumNet);
	pvseries->addMarSystem(peClust);

	/*************************************************************/

	if(synthetize) 
	{
		//create shredder
		synthNetCreate(&mng, outsfname);
		MarSystem *peSynth = mng.create("PeSynthetize", "synthNet");
		pvseries->addMarSystem(peSynth);
	}

	////////////////////////////////////////////////////////////////
	// update the controls
	////////////////////////////////////////////////////////////////
	pvseries->updctrl("Accumulator/accumNet/mrs_natural/nTimes", accSize);

	if (microphone_) 
	{
		pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/AudioSource/src/mrs_natural/inSamples", D);
		pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/AudioSource/src/mrs_natural/inObservations", 1);
	}
	else
	{
		pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/SoundFileSource/src/mrs_string/filename", sfName);
		pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/SoundFileSource/src/mrs_natural/inSamples", D);
		pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/SoundFileSource/src/mrs_natural/inObservations", 1);
	}

	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/Series/mixseries/SoundFileSource/noise/mrs_string/filename", noiseName);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/Series/mixseries/SoundFileSource/noise/mrs_natural/inSamples", D);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/Series/mixseries/Delay/noiseDelay/mrs_natural/delay", (mrs_natural) noiseDelay);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/Series/mixseries/Gain/noisegain/mrs_real/gain", .5);


	pvseries->updctrl("Accumulator/accumNet/Series/preNet/ShiftInput/si/mrs_natural/Decimation", D);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/ShiftInput/si/mrs_natural/WindowSize", Nw+1);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Windowing/wi/mrs_natural/size", N);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Windowing/wi/mrs_string/type", "Hanning");
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Windowing/wi/mrs_natural/zeroPhasing", 1);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/Shifter/sh/mrs_natural/shift", 1);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/PvFold/fo/mrs_natural/Decimation", D);
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/PeConvert/conv/mrs_natural/Decimation", D);      
	pvseries->updctrl("Accumulator/accumNet/Series/preNet/PeConvert/conv/mrs_natural/Sinusoids", S);  

	pvseries->setctrl("PeClust/peClust/mrs_natural/Sinusoids", S);  
	pvseries->setctrl("PeClust/peClust/mrs_natural/Clusters", C); 
	pvseries->setctrl("PeClust/peClust/mrs_natural/hopSize", D); 
	pvseries->updctrl("PeClust/peClust/mrs_string/similarityType", T); 

	//pvseries->update();

	if(synthetize)
	{
		synthNetConfigure (pvseries, sfName, outsfname, Nw, D, S, accSize);
	}

	//	cout << *pvseries;
	mrs_real globalSnr = 0;
	mrs_natural nb=0;
	while(1)
	{
		pvseries->tick();

		// ouput the seg snr
		if(synthetize)
		{
			mrs_real snr = pvseries->getctrl("Shredder/synthNet/Series/postNet/PeResidual/res/mrs_real/snr")->toReal();
			globalSnr+=snr;
			nb++;
			cout << "Frame " << nb << " SNR : "<< snr << endl;
		}

		if (!microphone_)
		{
			bool temp = pvseries->getctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/SoundFileSource/src/mrs_bool/notEmpty")->toBool();
			bool temp1 = accumNet->getctrl("Series/preNet/Fanin/fanin/SoundFileSource/src/mrs_bool/notEmpty")->toBool();
			bool temp2 = preNet->getctrl("Fanin/fanin/SoundFileSource/src/mrs_bool/notEmpty")->toBool();
			string fname = pvseries->getctrl("Accumulator/accumNet/Series/preNet/Fanin/fanin/SoundFileSource/src/mrs_string/filename")->toString();

			///*bool*/ temp = pvseries->getctrl("Accumulator/accumNet/Series/preNet/SoundFileSource/src/mrs_bool/notEmpty")->toBool();
			if (temp2 == false)
				break;
		}
	}
	if(synthetize_)
		cout << "Global SNR : " << globalSnr/nb << endl;

	// plot and save peak data
	peakSet = pvseries->getctrl("PeClust/peClust/mrs_realvec/peakSet")->toVec();


	ofstream peakFile;
	peakFile.open(filePeakName.c_str());
	if(!peakFile)
		cout << "Unable to open output Peaks File " << filePeakName << endl;
	peakFile << peakSet_;
	peakFile.close();
}





void clusterGroundThruth(realvec& peakSet, PeClusters& clusters, string fileName)
{

}

void clusterSynthetize(realvec& peakSet, string fileName)
{

}

void 
initOptions()
{
	cmd_options.addBoolOption("help", "h", false);
	cmd_options.addBoolOption("usage", "u", false);
	cmd_options.addNaturalOption("voices", "v", 1);
	cmd_options.addStringOption("filename", "f", EMPTYSTRING);
	cmd_options.addStringOption("noisename", "N", EMPTYSTRING);
	cmd_options.addStringOption("outputdirectoryname", "o", EMPTYSTRING);
	cmd_options.addStringOption("inputdirectoryname", "i", EMPTYSTRING);
	cmd_options.addNaturalOption("winsize", "w", winSize_);
	cmd_options.addNaturalOption("fftsize", "n", fftSize_);
	cmd_options.addNaturalOption("sinusoids", "s", nbSines_);
	cmd_options.addNaturalOption("bufferSize", "b", bopt);

	cmd_options.addBoolOption("analyse", "a", analyse_);
	cmd_options.addBoolOption("attributes", "A", attributes_);
	cmd_options.addBoolOption("ground", "g", ground_);
	cmd_options.addBoolOption("synthetize", "s", synthetize_);
	cmd_options.addBoolOption("clusterSynthetize", "S", clusterSynthetize_);
}


void 
loadOptions()
{
	helpopt_ = cmd_options.getBoolOption("help");
	usageopt_ = cmd_options.getBoolOption("usage");
	pluginName = cmd_options.getStringOption("plugin");
	fileName   = cmd_options.getStringOption("filename");
	inputDirectoryName = cmd_options.getStringOption("inputdirectoryname");
	outputDirectoryName = cmd_options.getStringOption("outputdirectoryname");
	noiseName = cmd_options.getStringOption("noisename");
	winSize_ = cmd_options.getNaturalOption("winsize");
	fftSize_ = cmd_options.getNaturalOption("fftsize");
	nbSines_ = cmd_options.getNaturalOption("sinusoids");
	bopt = cmd_options.getNaturalOption("bufferSize");

	analyse_ = cmd_options.getBoolOption("analyse");
	attributes_ = cmd_options.getBoolOption("attributes");
	ground_ = cmd_options.getBoolOption("ground");
	synthetize_ = cmd_options.getBoolOption("synthetize");
	clusterSynthetize_ = cmd_options.getBoolOption("clusterSynthetize");

}



int
main(int argc, const char **argv)
{
	MRSDIAG("sftransform.cpp - main");

	initOptions();
	cmd_options.readOptions(argc, argv);
	loadOptions();  

	vector<string> soundfiles = cmd_options.getRemaining();
	vector<string>::iterator sfi;


	string progName = argv[0];  


	if (helpopt_) 
		printHelp(progName);

	if (usageopt_)
		printUsage(progName);


	cerr << "peakClustering configuration (-h show the options): " << endl;
	cerr << "fft size (-n)      = " << fftSize_ << endl;
	cerr << "win size (-w)      = " << winSize_ << endl;
	cerr << "sinusoids (-s)     = " << nbSines_ << endl;
	cerr << "outFile  (-f)      = " << fileName << endl;
	cerr << "outputDirectory  (-o) = " << outputDirectoryName << endl;
	cerr << "inputDirectory  (-i) = " << inputDirectoryName << endl;

	// extract peaks and clusters
	// soundfile input 
	string sfname;
	if (soundfiles.size() != 0)   
	{
		// process several soundFiles
		for (sfi=soundfiles.begin() ; sfi!=soundfiles.end() ; sfi++)
		{
			FileName Sfname(*sfi);
			if(outputDirectoryName != EMPTYSTRING)
			{

				fileName = outputDirectoryName + "/" + Sfname.name() ;
				fileResName = outputDirectoryName + "/" + Sfname.nameNoExt() + "Res." + Sfname.ext() ;
				filePeakName = outputDirectoryName + "/" + Sfname.nameNoExt() + "Peak.txt" ;
				cout << fileResName << endl;
			}
			if(analyse_)
			{
				cout << "Phasevocoding " << Sfname.name() << endl; 
				clusterExtract(peakSet_, *sfi, fileName, noiseName, noiseDelay_, similarityType_, fftSize_, winSize_, hopSize_, nbSines_, nbClusters_, accSize_, synthetize_);
			}	
			// if ! peak data read from file
			if(peakSet_.getSize() == 0)
				peakSet_.read(filePeakName);
			if(peakSet_.getSize() == 0)
			{
				cout << "unable to load " << filePeakName << endl;
				exit(1);
			}

			MATLAB_PUT(peakSet_, "peaks");
			MATLAB_EVAL("plotPeaks(peaks)");


			// computes the cluster attributes

			if(attributes_)
			{
				PeClusters clusters(peakSet_);
				mrs_natural nbClusters=0;

				// compute ground truth
				if(ground_)
					clusterGroundThruth(peakSet_, clusters, fileName);

				clusters.selectBefore(noiseDelay_/hopSize_);
				updateLabels(peakSet_, clusters.getConversionTable());
			}
			// synthetize remaining clusters
			if(clusterSynthetize_)
				clusterSynthetize(peakSet_, fileResName);

			MATLAB_PUT(peakSet_, "peaks");
			MATLAB_EVAL("plotPeaks(peaks)");

		}
	}
	else
	{
		cout << "Using live microphone input" << endl;
		microphone_ = true;
		clusterExtract(peakSet_, "microphone", fileName, noiseName, noiseDelay_, similarityType_, fftSize_, winSize_, hopSize_, nbSines_, nbClusters_, accSize_, synthetize_);
	}



	exit(0);
}


