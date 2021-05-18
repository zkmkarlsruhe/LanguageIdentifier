#ifdef SAKADO_WAV_FILE_WRITER_BETA_H
#else
#define SAKADO_WAV_FILE_WRITER_BETA_H

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include<cstdio>
#include<string>

using namespace::std;

namespace sakado {

	class WavFileWriterBeta {
		FILE*fp;
	public:
		unsigned long FmtSize;
		unsigned short FmtID;
		unsigned short NumChannels;
		unsigned long SampleRate;
		unsigned long BytesPerSec;
		unsigned short BlockAlign;//(bytes per sample)*(channels)
		unsigned short BitsPerSample;
		unsigned long DataSize;
		unsigned short BytesPerSample;
		unsigned long NumData;
		string Filename;

		WavFileWriterBeta(string filename, unsigned long SampleRate, unsigned short BytesPerSample, unsigned long NumData) {

			char riff[] = "RIFF";
			char wave[] = "WAVE";
			char fmt[] = "fmt ";
			char data[] = "data";
			unsigned long bufl;
			unsigned long bufs;
			
			this->Filename = filename;
			fp = fopen(filename.c_str(), "wb");

			fwrite(riff, 1, 4, fp);
			bufl = 36 + (NumData * BytesPerSample);
			fwrite(&bufl, 1, 4, fp);
			fwrite(wave, 1, 4, fp);
			fwrite(fmt, 1, 4, fp);
			bufl = 16;
			fwrite(&bufl, 1, 4, fp);
			bufs = 1;
			fwrite(&bufs, 1, 2, fp);
			fwrite(&bufs, 1, 2, fp);
			bufl = SampleRate;
			fwrite(&bufl, 1, 4, fp);
			bufl = BytesPerSample * SampleRate;
			fwrite(&bufl, 1, 4, fp);
			bufs = BytesPerSample;
			fwrite(&bufs, 1, 2, fp);
			bufs = BytesPerSample * 8;
			fwrite(&bufs, 1, 2, fp);
			fwrite(data, 1, 4, fp);
			bufl = NumData * BytesPerSample;
			fwrite(&bufl, 1, 4, fp);

		}

		~WavFileWriterBeta() {
			if(fp != NULL)fclose(fp);
		}

		unsigned int Write(const void* buf, size_t elementSize, size_t elementCount) {
			return fwrite(buf, elementSize, elementCount, fp);
		}

		void Close() {
			if(fp != NULL)fclose(fp);
			fp = NULL;
		}

		//Not yet tested
		void Reopen() {
			if(fp == NULL)fp = fopen(Filename.c_str(), "ab");
		}
	};

}


#endif