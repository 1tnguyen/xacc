import numpy as np


class PulseOptParams:
    def __init__(self, nb_qubits=2, nb_segments=2, total_length=100.0):
        self.nBits = nb_qubits
        self.nSegs = nb_segments
        self.amplitude = []
        self.freq = []
        self.tSeg = []
        for qIdx in range(nb_qubits):
            # Amplitude in the range of [0.0, 1.0]
            self.amplitude.append(np.random.rand(nb_segments))
            # Assuming the frequency shift in the range of [-1.0, 1.0]
            self.freq.append(np.zeros(nb_segments))
            rand_segs = np.random.rand(nb_segments)
            # Time segments: using a random sequence to divide the total time into
            # time windows (sum up to the total length)
            segments = [
                total_length * r / np.sum(rand_segs) for r in rand_segs
            ]
            self.tSeg.append(segments)

    # Returns the pulse samples driving a particular qubit.
    # (constructed from the current parameters)
    def getPulseSamples(self, qubit, dt):
        amps = self.amplitude[qubit]
        freqs = self.freq[qubit]
        time_windows = self.tSeg[qubit]
        # Note: this is hardcoded to square pulses (step function)
        time_list = np.arange(0.0, np.sum(time_windows), dt)
        # which time segment are we on?
        segment_idx = 0
        pulse_vals = []
        next_switching_time = time_windows[segment_idx]
        for time in time_list:
            if time > next_switching_time:
                # Switch to the next window
                segment_idx = segment_idx + 1
                next_switching_time = next_switching_time + time_windows[
                    segment_idx]
            # Retrieve the amplitude and frequencies
            amp = amps[segment_idx]
            freq = freqs[segment_idx]

            # Compute the sample:
            # Note: We treat the frequency param here [-1, 1]
            # as modulation before LO mixing (at resonance frequency)
            # We can change the LO freq. for each pulse, but for simplicity,
            # we pre-modulate the pulse (AER simulator doesn't support per-pulse LO freq. change)
            # i.e.
            # signal = pulse * exp(-i * (w0 + dw) * t) = pulse * exp(-i * dw * t) * exp(-i * w0 * t)
            # hence, pulse at different frequency can be transformed into [pulse * exp(-i * dw * t)]
            # to create LO freq. shift effect.
            pulse_val = amp * np.exp(-1j * 2.0 * np.pi * freq * time)
            pulse_vals.append(pulse_val)
        return pulse_vals


import xacc, json

# Query backend info (dt)
qpu = xacc.getAccelerator("aer:ibmq_armonk", {"sim-type": "pulse"})
backend_properties = qpu.getProperties()
config = json.loads(backend_properties["config-json"])
dt = config["dt"]

# Simple test: single qubit
ham = xacc.getObservable("pauli", "1.0 Z0")
# Pulse parameter:
# Simple: single segment, i.e. square pulse.
pulse_opt = PulseOptParams(nb_qubits=1, nb_segments=1, total_length=100.0)
provider = xacc.getIRProvider("quantum")


# Optimization function:
# Note: We need to unpack the flattened x array into {amp, freq, time segment}.
def pulse_opt_func(x):
    # This is a one-segment optimization:
    amp = x[0]
    freq = x[1]
    # Update the params in the PulseOptParams
    pulse_opt.freqs = [[freq]]
    pulse_opt.amplitude = [[amp]]
    # Construct the pulse program:
    program = provider.createComposite("vqe_pulse_composite")
    # Create the pulse instructions
    pulse_inst = provider.createInstruction(
        "pulse", [0], [], {
            "channel": "d0",
            "samples": pulse_opt.getPulseSamples(0, dt)
        })
    program.addInstruction(pulse_inst)
    # Observe the pulse program:
    fs_to_exe = ham.observe(program)
    # Execute
    energy = 0.0
    for i in range(len(fs_to_exe)):
        fs = fs_to_exe[i]
        term = ham.getNonIdentitySubTerms()[i]
        coeff = 0.0
        for op in term:
            coeff = op[1].coeff().real
        buffer = xacc.qalloc(1)
        qpu.execute(buffer, fs)
        # print(buffer)
        # print("Exp-Z =", buffer.getExpectationValueZ())
        energy = energy + coeff * buffer.getExpectationValueZ()
    print("Energy(", x, ") =", energy)
    return energy


# Run the optimization loop:
# Optimizer:
# Single segment: 1 amplitude and 1 freq
# Make sure we don't create a pulse with amplitude > 1.0 (error)
optimizer = xacc.getOptimizer('nlopt', {
    "lower-bounds": [-1.0, -1.0],
    "upper-bounds": [1.0, 1.0],
    "maxeval": 100
})
result = optimizer.optimize(pulse_opt_func, 2)
print(result)

# Not changing the freq., just varying the amplitude:
# Should see a Rabi oscillation....
# for ampl in np.linspace(0.0, 1.0, 100):
#     val = pulse_opt_func([ampl, 0.0])
#     print("E(", ampl, ") =", val)
