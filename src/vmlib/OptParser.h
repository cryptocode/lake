#ifndef LAKE_CORE_OPT_PARSER_H
#define LAKE_CORE_OPT_PARSER_H

#include <vector>
#include <map>
#include <string>
#include <cstring>

using namespace std;

namespace lake
{
    constexpr unsigned int DESCRIPTION_INDENT{25};

	/**
	 * An option
	 */
	struct Opt
	{

		/**
		 * Option
		 * 
		 * @param shortKey Short key, e.g. "v" => "-v"
		 * @param longKey Long key, e.g. "version" => "--input"
		 * @param description Description of the option
		 * @param cardinality Option value cardinality. 0 = no value (option is a flag),
		 *                    -1 = any number of values. > 0 = number
		 *                    of values accepted (1 for single value option, etc)
		 */
		Opt(string& shortKey, string& longKey, const char* description = 0, int cardinality = 0)
		{
			this->used = false;
			this->shortKey = shortKey;
			this->longKey = longKey;
			this->cardinality = cardinality;

			if(description != 0)
				this->description = description;
		}

		/**
		 * Get option values
		 * 
		 * @return vector<string>&
		 */
		vector<string>& getValues()
		{
			return values;
		}

		bool used;
		int cardinality;

		string shortKey;
		string longKey;
		string description;
		vector<string> values;
	};

	/**
	 * Command line options parser
	 */
	class OptParser
	{
	public:

		OptParser()
		{
		}

		~OptParser()
		{
			for(size_t i=0; i < allOptions.size(); i++)
			{
				Opt* opt = allOptions.at(i);
				delete opt;
			}
		}

		/**
		 * Parse options, such as:
		 * 
		 *      -h -v --version --files a b "c d" e\ f -b -c 2 --num 45
		 * 
		 * @param argc as passed to main (except program name)
		 * @param argv as passed to main (except program name)
		 * @return 0 on success, else error string.
		 */
		const char* parse(int argc, const char *argv[])
		{
			static char error[1024];
			error[0] = 0;

			Opt* opt = 0;
			string key;

			for(int i=1; i < argc; i++)
			{
				// Option key
				if(argv[i][0] == '-')
				{
					// Check previous
					if(opt && (opt->cardinality == -1 || opt->cardinality > 0) && opt->values.size() == 0)
					{
						snprintf(error, 1023, "Option '%s' requires a value", key.c_str());
						return error;
					}

					// Reset
					opt = 0;

					if(argv[i][1] == '-')
					{
						const char* longOpt = &argv[i][2];
						key = longOpt;
					}
					else
					{
						char shortOpt = argv[i][1];
						key = shortOpt;
					}

					opt = getOpt(key);

					if(opt == 0)
					{
                        snprintf(error, 1023, "'%s' is not a valid option", key.c_str());
						return error;
					}
					else
					{
						opt->used = true;
					}
				}
				// Value
				else
				{
					string value = argv[i];

					if(!opt)
					{
                        snprintf(error, 1023, "Value without option: %s", argv[i]);
						return error;
					}
					else if(opt->cardinality == 0)
					{
                        snprintf(error, 1023, "Option '%s' does not take any values", key.c_str());
						return error;                    
					}
					else if(opt->cardinality != -1 && (int)opt->values.size()+1 > opt->cardinality)
					{
                        snprintf(error, 1023, "Option '%s' takes a maximum of %d value(s)", key.c_str(), opt->cardinality);
						return error;                    
					}
					else
					{
						opt->values.push_back(value);
					}
				}
			}

			// Check last option
			if(opt && (opt->cardinality == -1 || opt->cardinality > 0) && opt->values.size() == 0)
			{
                snprintf(error, 1023, "Option '%s' requires a value", key.c_str());
				return error;
			}

			return 0;
		}


		/**
		 * Add option
		 * 
		 * @param longOpt Long key, e.g. "version" => "--version"
		 * @param shortOpt Short key, e.g. "v" => "-v"
		 * @param description Description of the option
		 * @param cardinality Option value cardinality. 0 = no value (option is a flag),
		 *                    -1 = any number of values. > 0 = number
		 *                    of values accepted (1 for single value option, etc)
		 * @return Option
		 */
		Opt* addOption(const char* longOpt, const char* shortOpt, const char* description, int cardinality = 0)
		{
			string keyShort = shortOpt;
			string keyLong = longOpt;

			Opt* opt = new Opt(keyShort, keyLong, description, cardinality);

			if(keyShort.length() > 0)
				options[keyShort] = opt;

			if(keyLong.length() > 0)
				options[keyLong] = opt;

			allOptions.push_back(opt);

			return opt;
		}

		/**
		 * Print usage information based on the added options
		 */
		void printUsage()
		{
			printf("Usage:\n\n");

			for(size_t i=0; i < allOptions.size(); i++)
			{
				Opt* opt = allOptions.at(i);
				char line[1024];

				strcpy(line, "  ");
				if(opt->shortKey.length() > 0)
				{
					snprintf(&line[strlen(line)], 256, "-%s", opt->shortKey.c_str());

					if(opt->longKey.length() > 0)
						strncat(line, ", ", 2);
				}

				if(opt->longKey.length() > 0)
					snprintf(&line[strlen(line)], 256, "--%s", opt->longKey.c_str());

				int len = strlen(line);
				for(int i=len; i < DESCRIPTION_INDENT; i++)
					strncat(line, " ", 1);
			
				printf("%s%s\n", line, opt->description.c_str());
			}

			printf("\n");
		}

		/**
		 * Get option
		 * 
		 * @param key Long or short option key
		 * @return Opt* Option, or nullptr if the option is not used or invalid.
		 */
		Opt* getOption(const std::string& key)
		{
			Opt* opt = getOpt(key);

			return (opt && opt->used) ? opt : 0;
		}

        std::string getFirstValue(const std::string& key)
        {
            return getOption(key)->values.at(0);
        }

		/**
		 * Returns true if the given option is present
		 */
		 bool hasOption(const std::string& key)
		 {
			return getOption(key) != nullptr;
		 }

		/**
		 * Get the number of options
		 */
		int getOptionCount() const
		{
			return options.size();
		}

	private:

		/**
		 * Get option
		 *
		 * @param key Option
		 * @return Option, or NULL if not found
		 */
		Opt* getOpt(const std::string& key)
		{
			Opt* res = 0;

			// We use this idiom instead of var[key], because the 
			// subscript semantics of std::map is severely broken (it 
			// will create an entry if the key is  not found.)
			std::map<std::string, Opt*>::iterator itr = options.find(key);

			if(itr != options.end())
				res = itr->second;

			return res;
		}

		map<string,Opt*> options;
		vector<Opt*> allOptions;
	};


}// ns

#endif // LAKE_CORE_OPT_PARSER_H
