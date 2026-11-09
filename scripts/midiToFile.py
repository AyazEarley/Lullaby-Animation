import pretty_midi

INPUT_PATH = "lullaby.mid"
OUTPUT_PATH = "notes.txt"

midi = pretty_midi.PrettyMIDI(INPUT_PATH)

with open(OUTPUT_PATH, "w") as f:
    for inst in midi.instruments:
        for n in inst.notes:
            f.write(f"{n.pitch} {n.start} {n.end}\n")