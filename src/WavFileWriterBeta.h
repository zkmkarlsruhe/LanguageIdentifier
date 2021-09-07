// adapted from https://github.com/KojiroSakado/WavFileWriterBeta
// Kojiro Sakado 2019
// ref: http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
#pragma once

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include <cstdio>
#include <string>

namespace sakado {

/// very basic wave file writer using c stdio
class WavFileWriterBeta {

	public:

		std::string filename;

		/// open file for a fixed-length number of samples
		WavFileWriterBeta(std::string filename, unsigned short numChannels,
			                                    unsigned long sampleRate,
			                                    unsigned short bytesPerSample,
			                                    unsigned long numSamples) :
			                                    filename(filename) {
			char riff[] = "RIFF";
			char wave[] = "WAVE";
			char fmt[]  = "fmt ";
			char data[] = "data";
			unsigned long numData = numChannels * bytesPerSample * numSamples;
			unsigned long bufl;
			unsigned long bufs;

			fp = fopen(filename.c_str(), "wb");

			// header chunk, 12 bytes
			fwrite(riff, 1, 4, fp);
			bufl = 36 + (numData);                  // chunk data len
			fwrite(&bufl, 1, 4, fp);
			fwrite(wave, 1, 4, fp);

			// (basic) format chunk, 24 bytes
			fwrite(fmt, 1, 4, fp);
			bufl = 16;                              // chunk data len
			fwrite(&bufl, 1, 4, fp);
			bufs = 1;                               // format tag
			fwrite(&bufs, 1, 2, fp);
			bufs = numChannels;                     // number of channels
			fwrite(&bufs, 1, 2, fp);
			bufl = sampleRate;                      // sample rate
			fwrite(&bufl, 1, 4, fp);
			bufl = bytesPerSample * sampleRate;     // average bytes per second
			fwrite(&bufl, 1, 4, fp);
			bufs = bytesPerSample;                  // bytes per frame
			fwrite(&bufs, 1, 2, fp);
			bufs = bytesPerSample * 8;              // bits per sample
			fwrite(&bufs, 1, 2, fp);

			// data chunk
			fwrite(data, 1, 4, fp);
			bufl = numData * bytesPerSample;        // chunk data length
			fwrite(&bufl, 1, 4, fp);
		}

		~WavFileWriterBeta() {
			close();
		}

		/// write nitems of size bytes from buf to file
		/// returns number of items written
		size_t write(const void *buf, size_t size, size_t nitems) {
			return fwrite(buf, size, nitems, fp);
		}

		/// close file
		void close() {
			if(fp) {
				fclose(fp);
				fp = NULL;
			}
		}

		/// reopen file to append, not yet tested
		void reopen() {
			if(!fp) {
				fp = fopen(filename.c_str(), "ab");
			}
		}

	private:

		FILE *fp = NULL;
};

} // namespace
