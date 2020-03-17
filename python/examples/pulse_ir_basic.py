import xacc
import numpy as np

# Load the backend Json file which contains pulse and cmd-def library
with open('quantum/gate/ir/tests/files/test_backends.json', 'r') as backendFile:
    jjson = backendFile.read()

if xacc.hasAccelerator('QuaC') is True:
    model = xacc.createPulseModel()
    qpu = xacc.getAccelerator('QuaC', {'system-model': model.name()})
    # Contribute pulse instructions and pulse composites (cmd-def) to the instruction registry
    qpu.contributeInstructions(jjson);

    provider = xacc.getIRProvider('quantum')
    pulseCircuit = provider.createComposite('Bell')
    # Retrieve pulse instructions from the IR provider
    hadamard = provider.createInstruction('pulse::u2_0', [0], [0.0, np.pi])
    cnot = provider.createInstruction('pulse::cx_0_1', [0,1])
    pulseCircuit.addInstructions([hadamard, cnot])

    print(pulseCircuit)




