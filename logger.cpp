// logger.cpp

// Logging class inspired by Dr. Dobbs: http://www.drdobbs.com/cpp/logging-in-c/201804215?pgno=3

#include <ctime>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <mutex>
#include <unordered_set>

class ScopedLock {
public:
	ScopedLock(std::mutex & mtx) : mtx_(mtx) {
		mtx_.lock();
	}
	~ScopedLock() {
		mtx_.unlock();
	}
protected:
	std::mutex & mtx_;
};

/* Log levels in descending severity order */

enum log_level_t {ERROR, WARN, INFO, DEBUG};
const char * EnumToString[4] = {"ERROR", "WARN", "INFO", "DEBUG"};

class OutputPolicy {
public:
	inline static void SetStream(FILE * pStream) {
		ScopedLock lock(mtx_);
		StreamImpl() = pStream;
	}

	inline static void Output(const std::string & msg) {
		ScopedLock lock(mtx_);
		FILE * pStream = StreamImpl();
		if (pStream) {
			std::fprintf(pStream, "%s", msg.c_str()); std::fflush(pStream);
		}
	}
	~OutputPolicy() {
		fclose(StreamImpl());
	}
private:
	inline static FILE* & StreamImpl() {
		static FILE * pStream = stderr;
		return pStream;
	}

	static std::mutex mtx_;
};

std::mutex OutputPolicy::mtx_;

template<class OutputPolicy>
class Log {
public:
	static size_t reportingLevel;
	virtual ~Log() {
		OutputPolicy::Output(oss_.str());
	}
	Log() { }
	std::ostringstream & Get(unsigned level) {
		ScopedLock lock(preamble_mtx_);
		preamble_(level);
		return oss_;
	}

protected:
	virtual void preamble_(unsigned level) {
	/* NOTE: Race Condition - ctime uses static buffer, function not reentrant. Solution could be to use guard lock */
	/* Resource: https://wiki.sei.cmu.edu/confluence/display/c/CON33-C.+Avoid+race+conditions+when+using+library+functions */
		time_t currtime = time(nullptr);
		char * time_cstr = ctime(&currtime);
		time_cstr[strlen(time_cstr) - 1] = '\0';
		oss_ << " - " << EnumToString[level] << ": " << time_cstr << ": ";
	}

private:
	static std::mutex preamble_mtx_;
	std::ostringstream oss_;

};

/* Initialize Preamble Mutex */

template<class OutputPolicy>
std::mutex Log<OutputPolicy>::preamble_mtx_;

/* Set Reporting Level - Anything above and including set value will be recorded */

template<class OutputPolicy>
size_t Log<OutputPolicy>::reportingLevel = INFO;

/* Macro to make logging easier for programmer */

typedef Log<OutputPolicy> logFILE;

#define LOG(level) \
if (level > logFILE::reportingLevel) ; \
else logFILE().Get(level)

/* Test function */

void vowels() {
	std::unordered_set<char> vowels {'a', 'e', 'i', 'o', 'u'};
	for (char c = 'a'; c <= 'z'; c++) {
		if (vowels.find(c) != vowels.end()) {
			LOG(INFO) << "Vowels(): " << c << std::endl;
		}
	}
}

void consonants() {
	std::unordered_set<char> vowels {'a', 'e', 'i', 'o', 'u'};
	for (char c = 'a'; c <= 'z'; c++) {
		if (vowels.find(c) == vowels.end()) {
			LOG(INFO) << "Consonants(): " << c << std::endl;
		}
	}
}

/* Driver */

int main() {
	logFILE Log;
	OutputPolicy::SetStream(fopen("app.log", "a"));

	std::thread c(consonants);
	std::thread v(vowels);

	c.join();
	v.join();

	return 0;
}