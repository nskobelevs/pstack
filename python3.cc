#include <python3.7/Python.h>
#include <python3.7/frameobject.h>
#include <python3.7/longintrepr.h>
#include <python3.7/longintrepr.h>
#include "libpstack/python.h"

template<> std::set<const PythonTypePrinter<3> *> PythonTypePrinter<3>::all = std::set<const PythonTypePrinter<3> *>();
template <>
char PythonTypePrinter<3>::pyBytesType[] = "PyBytes_Type";

/*
 * Process one python interpreter in the process at remote address ptr
 * Returns the address of the next interpreter on on the process's list.
 */
template <>
Elf::Addr
PythonPrinter<3>::printInterp(Elf::Addr ptr, bool showModules)
{
    // these are the first two fields in PyInterpreterState - next and tstate_head.
    struct State {
        Elf::Addr next;
        Elf::Addr head;
        Elf::Addr modules;
    };
    State state;
    proc.io->readObj(ptr, &state.next);
    proc.io->readObj(ptr + sizeof(void *), &state.head);
    proc.io->readObj(ptr + sizeof(void *)*2 + 16, &state.modules);
    os << "---- interpreter @" << std::hex << ptr << std::dec << " -----" << std::endl ;
    for (Elf::Addr tsp = state.head; tsp; ) {
        tsp = printThread(tsp);
        os << std::endl;
    }
    if (showModules) {
       os << "---- modules:" << std::endl;
       print(state.modules);
    }
    return state.next;
}

class BoolPrinter : public PythonTypePrinter<3> {
    Elf::Addr print(const PythonPrinter<3> *pc, const PyObject *pyo, const PyTypeObject *, Elf::Addr) const override {
        auto pio = (const _longobject *)pyo;
        pc->os << (pio->ob_digit[0] ? "True" : "False");
        return 0;
    }
    const char *type() const override { return "PyBool_Type"; }
    bool dupdetect() const override { return false; }
};
static BoolPrinter boolPrinter;

template<>
void PythonPrinter<3>::findInterpHeadFallback() {
    libpython = nullptr;

    Elf::Addr _PyRuntime;
    std::tie(libpython, libpythonAddr, _PyRuntime) = proc.findSymbolDetail("_PyRuntime", false);
        
    interp_head = _PyRuntime + sizeof(int) * 2  + sizeof(void *)*2;
}

#include "python.tcc"

template struct PythonPrinter<3>;
