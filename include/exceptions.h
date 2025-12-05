#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

// Source - https://stackoverflow.com/a
// Posted by James, modified by community. See post 'Timeline' for change history
// Retrieved 2025-12-05, License - CC BY-SA 4.0

#include <stdexcept>
#include <string>
class NotImplemented : public std::logic_error {
 private:
    std::string _text;

    NotImplemented(const char *message, const char *function)
        : std::logic_error("Not Implemented") {
        _text = message;
        _text += " : ";
        _text += function;
    }

 public:
    NotImplemented() : NotImplemented("Not Implememented", __FUNCTION__) {}

    explicit NotImplemented(const char *message) : NotImplemented(message, __FUNCTION__) {}

    virtual const char *what() const throw() { return _text.c_str(); }
};

#endif
