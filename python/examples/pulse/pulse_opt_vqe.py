import numpy as np

class PulseOptParams:
    def __init__(self, nb_qubits = 2, nb_segments=2, total_length = 100.0):
        self.nBits = nb_qubits
        self.nSegs = nb_segments
        self.amplitude = []
        self.freq = []
        self.tSeg = []
        for qIdx in range(nb_qubits):
            # Amplitude in the range of [0.0, 1.0]
            self.amplitude.append(np.random.rand(nb_segments))
            # Assuming the frequency shift in the range of [-1.0, 1.0]
            self.freq.append((np.random.rand(nb_segments) - 0.5)*2.0)
            rand_segs = np.random.rand(nb_segments)
            # Time segments: using a random sequence to divide the total time into 
            # time windows (sum up to the total length)
            segments = [total_length * r/np.sum(rand_segs) for r in rand_segs]
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
                next_switching_time = next_switching_time + time_windows[segment_idx]
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
            pulse_val = amp * np.exp(-1j*2.0*np.pi*freq*time)
            pulse_vals.append(pulse_val)
        return pulse_vals


import xacc
# Simple test: single qubit
pulse_opt = PulseOptParams(nb_qubits=1)
provider = xacc.getIRProvider("quantum")
program = provider.createComposite("gaussian")
# Create the pulse instructions
# Just use random initial pulse, we eventually need to put this into an optimization loop to update parameters...
pulse_inst = provider.createInstruction("pulse", [0], [], { "channel" : "d0", "samples": pulse_opt.getPulseSamples(0, 0.22)})
program.addInstruction(pulse_inst)
# Measure instructions (to be lowered to pulses)
m0 = provider.createInstruction("Measure", [0])
program.addInstruction(m0)

# Execute on the Aer simulator
qpu = xacc.getAccelerator("aer:ibmq_armonk", {"sim-type": "pulse"})
buffer = xacc.qalloc(1)
qpu.execute(buffer, program)
print(buffer)


