#!/usr/bin/env ruby

class Modulator
  # When we're sending MARK bits, we have 1.0 complete sine waves in
  # each bit.
  PHASE_SPEED_MARK = 1.0

  # When we're sending SPACE bits, we have 2200/1200 complete sine
  # waves in each bit.
  PHASE_SPEED_SPACE = 2200.0/1200.0


  # The current absolute time
  attr_accessor :cur_pos

  # The current phase position in the waveform
  attr_accessor :cur_phase

  # The phase speed of the symbol we're currently sending
  attr_accessor :cur_speed


  def initialize()
    @cur_pos = 0
    @cur_phase = 0
    @cur_speed = PHASE_SPEED_MARK
  end

  def send_bit(i)
    if (i == 1)
      send_one()
    else
      send_zero()
    end
  end

  def send_one()
    # Do nothing for a logical 1
  end

  def send_zero()
    # Toggle the frequency we're sending
    @cur_speed = (@cur_speed == PHASE_SPEED_MARK ? PHASE_SPEED_SPACE : PHASE_SPEED_MARK)
  end

  def get_zeros_for_bit()
    # Fetch the times at which there are zeros in the current bit

    retval = []

    # We have zeros every 0.5 phase units
    phase_to_next_zero = 0.5-@cur_phase.modulo(0.5)

    # Now, how long does it take to move that far?
    dt = phase_to_next_zero / @cur_speed

    # We want to send 1.0 time units of data
    to_send = 1.0

    # If we're on that spot, it's already been emitted.
    if (!dt.zero?)
      @cur_pos += dt
      retval.push(@cur_pos)

      @cur_phase += phase_to_next_zero

      to_send -= dt
    end

    # Now, how long does it take to go 180degrees?
    halfstep = 0.5/@cur_speed

    while (to_send >= halfstep)
      @cur_pos += halfstep
      @cur_phase += 0.5

      retval.push(@cur_pos)

      to_send -= halfstep
    end

    if (!to_send.zero?)
      # How far does our current speed go in that time?
      dphase = to_send * @cur_speed

      # Move phase forward that much
      @cur_phase += dphase

      # Move time position forward by to_send
      @cur_pos += to_send
    end

    return retval
  end
end

phase = 0;

while (phase < 1.0)
  m = Modulator.new()
  m.cur_phase = phase
  phase += 5.0/360.0

  bits = [1,0,1,1,1,1,1,1,0].map { |i| m.send_bit(i); m.get_zeros_for_bit() }

  puts bits.map { |a| a.join("\n") }.join("\n") + "\n9999999"
end
