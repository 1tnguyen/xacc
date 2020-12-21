import xacc
from pelix.ipopo.decorators import ComponentFactory, Property, Requires, Provides, \
    Validate, Invalidate, Instantiate

@ComponentFactory("cirq_three_qubit_merge_pass_factory")
@Provides("irtransformation")
@Property("_irtransformation", "irtransformation", "cirq_three_qubit_merge")
@Property("_name", "name", "cirq_three_qubit_merge")
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
        # Count gates:
        n_1q = 0
        n_2q = 0
        for inst in program.getInstructions():
            if len(inst.bits()) == 1:
                n_1q = n_1q + 1
            if len(inst.bits()) == 2:
                n_2q = n_2q + 1
        # Map CompositeInstruction program to OpenQasm string
        openqasm_compiler = xacc.getCompiler('staq')
        src = openqasm_compiler.translate(program).replace('\\','')

        # Create a QuantumCircuit
        circuit = QuantumCircuit.from_qasm_str(src)
        
        if circuit.n_qubits == 3:
            # Remove final measurements (to convert to matrix)
            pass_manager = PassManager()
            pass_manager.append(RemoveFinalMeasurements())
            out_circuit = transpile(circuit, pass_manager=pass_manager)
            circuit_op = Operator(out_circuit)   
            #print(circuit_op.data)

            circ_mat = circuit_op.data
            a, b, c = cirq.LineQubit.range(3)
            operations = cirq.optimizers.three_qubit_matrix_to_operations(a, b, c, circ_mat)
            out_src = str(cirq.QasmOutput(operations, [a,b,c]))
            #print(out_src)
            # Map the output to OpenQasm and map to XACC IR
            out_prog = openqasm_compiler.compile(out_src, accelerator).getComposites()[0]
            
            # Count gates after:
            n_1q_after = 0
            n_2q_after = 0
            for inst in out_prog.getInstructions():
                if len(inst.bits()) == 1:
                    n_1q_after = n_1q_after + 1
                if len(inst.bits()) == 2:
                    n_2q_after = n_2q_after + 1
            
            # update the given program CompositeInstruction reference
            # if the number of 2-q gates is reduced
            # or the number of 2-q gates is the same and the number of 1-q gates are reduced.
            if (n_2q_after < n_2q) or (n_2q_after < n_2q == n_2q and n_1q_after < n_1q):
                program.clear()
                for inst in out_prog.getInstructions():
                    program.addInstruction(inst)

        return