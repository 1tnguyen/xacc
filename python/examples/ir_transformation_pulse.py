# Demonstrate low-level pulse optimization API
# via direct unitary matrix input.
import xacc, sys, numpy as np

xacc.set_verbose(True)

qpu = xacc.getAccelerator('aer:ibmq_armonk')

# Create the CompositeInstruction containing a
# single X instruction
provider = xacc.getIRProvider('quantum')
circuit = provider.createComposite('U')
x = provider.createInstruction('X', [0])
circuit.addInstruction(x)

irt = xacc.getIRTransformation('quantum-control')
#'eps' is step size multiplier
irt.apply(circuit, qpu, {'method': 'GRAPE', 'max-time': 40.0, 'eps': 2.0})

print(circuit)
# Execute the pulse program...
# This is not correct ...
buffer = xacc.qalloc(1)
qpu.execute(buffer, circuit)
            
