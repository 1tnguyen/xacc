import xacc
from pelix.ipopo.decorators import ComponentFactory, Property, Requires, Provides, \
    Validate, Invalidate, Instantiate

@ComponentFactory("cirq_three_qubit_merge_pass_factory")
@Provides("irtransformation")
@Property("_irtransformation", "irtransformation", "cirq_three_qubit_merge_pass")
@Property("_name", "name", "cirq_three_qubit_merge_pass")
@Instantiate("cirq_three_qubit_merge_pass_instance")
class CirqMerge3qBlockIRTransformation(xacc.IRTransformation):
    def __init__(self):
        xacc.IRTransformation.__init__(self)

    def type(self):
        return xacc.IRTransformationType.Optimization

    def name(self):
        return 'cirq_three_qubit_merge'

    def apply(self, program, accelerator, options):
        from qiskit import QuantumCircuit, transpile
        from qiskit.transpiler import PassManager
        from qiskit.transpiler.passes import RemoveFinalMeasurements
        from qiskit.quantum_info import Operator
        import cirq

        # Map CompositeInstruction program to OpenQasm string
        openqasm_compiler = xacc.getCompiler('openqasm')
        src = openqasm_compiler.translate(program).replace('\\','')

        # Create a QuantumCircuit
        circuit = QuantumCircuit.from_qasm_str(src)
        
        if circuit.n_qubits == 3:
            # Remove final measurements (to convert to matrix)
            pass_manager = PassManager()
            pass_manager.append(RemoveFinalMeasurements())
            out_circuit = transpile(circuit, pass_manager=pass_manager)
            circuit_op = Operator(out_circuit)   
            print(circuit_op.data)

            circ_mat = circuit_op.data
            a, b, c = cirq.LineQubit.range(3)
            operations = cirq.optimizers.three_qubit_matrix_to_operations(a, b, c, circ_mat)
            out_src = cirq.QasmOutput(operations, [a,b,c])
            #print(out_src)
            # Map the output to OpenQasm and map to XACC IR
            out_src = '__qpu__ void '+program.name()+'(qbit q) {\n'+out_src+"\n}"
            out_prog = openqasm_compiler.compile(out_src, accelerator).getComposites()[0]
            # update the given program CompositeInstruction reference
            program.clear()
            for inst in out_prog.getInstructions():
                program.addInstruction(inst)

        return








