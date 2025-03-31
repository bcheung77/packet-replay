#ifndef PACKET_REPLAY_PYTHON_API_H
#define PACKET_REPLAY_PYTHON_API_H

#include <stdint.h>

#include <string>

namespace packet_replay
{
    class PythonApi;

    typedef struct _object PyObject;

    /**
     * Base class for objects making embedded Python calls
     */
    class PythonCall
    {
        protected:
            PyObject* py_func_;
            std::string func_name_;

            PythonCall(const std::string& script_file, const std::string& func_name);
            virtual ~PythonCall();
    };    

    /**
     * A wrapper to call Python to validate packets
     */
    class ValidatePythonCall : public PythonCall {
        public:
            /**
             * @param expected buffer containing the expected packet
             * @param expected_len the length of the expected packet
             * @param actual buffer containing the packet under test
             * @param actual_len the length of the actual packet
             */
            bool validate(const uint8_t* expected, int expected_len, const uint8_t* actual, int actual_len);

        private:
            ValidatePythonCall(const std::string& script_file, const std::string& func_name) : PythonCall(script_file, func_name) {
            }

        friend PythonApi;
    };    

    /**
     * Singleton to use for calling Python.  This is not thread safe so first getInstance must be called before threading.
     */
    class PythonApi
    {
        private:
            static PythonApi* instance__;
            PythonApi();

        public:

            ~PythonApi();

            PythonApi(PythonApi &other) = delete;
            void operator=(const PythonApi &) = delete;
            
            static PythonApi* getInstance();

            /**
             * Create an embedded Python call that compares and validates packets.  
             * 
             * @param script_file the path to the Python module contain the function call
             * @param func_name the name of the function to call 
             */
            ValidatePythonCall* createValidateCall(const std::string& script_file, const std::string& func_name) {
                return new ValidatePythonCall(script_file, func_name);
            }
    };    

} // namespace packet_replay

#endif