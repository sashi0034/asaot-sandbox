local function mix32(x)
  x = x ~ (x >> 16)
  x = (x * 0x7feb352d) & 0xffffffff
  x = x ~ (x >> 15)
  x = (x * 0x846ca68b) & 0xffffffff
  x = x ~ (x >> 16)
  return x & 0xffffffff
end

local function mix_checksum(acc, value)
  return (acc ~ (value + 0x9e3779b9 + ((acc << 6) & 0xffffffffffffffff) + (acc >> 2))) & 0xffffffffffffffff
end

local function rotr(x, n)
  return ((x >> n) | (x << (32 - n))) & 0xffffffff
end

local sha256_k = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
}

local function fib_recursive_impl(n)
  if n < 2 then return n end
  return fib_recursive_impl(n - 1) + fib_recursive_impl(n - 2)
end

local function solve_queen_impl(row, n, cols, diag1, diag2)
  if row == n then return 1 end
  local available = ((1 << n) - 1) & (~(cols | diag1 | diag2))
  local solutions = 0
  while available ~= 0 do
    local bit = available & (-available)
    available = available - bit
    solutions = solutions + solve_queen_impl(row + 1, n, cols | bit, (diag1 | bit) << 1, (diag2 | bit) >> 1)
  end
  return solutions
end

local function work_dictionary(seed)
  local map = {}
  local acc = 0
  for i = 1, 3000 do
    local key = mix32(i + seed * 131) & 4095
    map[key] = (i ~ (key << 1))
  end
  for i = 1, 3000 do
    local key = mix32(i + seed * 131 + 7000) & 4095
    local v = map[key]
    if v ~= nil then acc = (acc + v) & 0xffffffffffffffff end
  end
  return acc ~ (#map)
end

local function work_exp_loop(seed)
  local sum = 0.0
  for i = 1, 40000 do
    local x = ((i + seed) % 2048 + 1) * 0.00075
    sum = sum + math.exp(x)
  end
  return math.floor(sum * 1000.0)
end

local function work_fibonacci_loop(seed)
  local a = seed
  local b = seed + 1
  local acc = 0
  for _ = 1, 250000 do
    local c = a + b
    a = b
    b = c
    acc = acc ~ c
  end
  return mix_checksum(acc, a ~ b)
end

local function work_fibonacci_recursive(seed)
  return fib_recursive_impl(31 + (seed & 1))
end

local function work_float2string(seed)
  local acc = 0
  for i = 1, 12000 do
    local value = math.sin((i + seed) * 0.01) * 10000.0
    local text = string.format("%.9f", value)
    acc = mix_checksum(acc, #text + string.byte(text, 1))
  end
  return acc
end

local function work_mandelbrot(seed)
  local size = 96
  local offset = (seed & 7) * 0.0001
  local acc = 0
  for y = 0, size - 1 do
    for x = 0, size - 1 do
      local cr = (x / size) * 3.0 - 2.0 + offset
      local ci = (y / size) * 2.0 - 1.0 - offset
      local zr, zi, iter = 0.0, 0.0, 0
      while ((zr * zr + zi * zi) <= 4.0 and iter < 80) do
        local next_r = zr * zr - zi * zi + cr
        zi = 2.0 * zr * zi + ci
        zr = next_r
        iter = iter + 1
      end
      acc = acc + iter
    end
  end
  return acc
end

local function work_n_bodies(seed)
  local bodies = {
    {x = 0.0, y = 0.0, z = 0.0, vx = 0.0, vy = 0.0, vz = 0.0, mass = 10.0},
    {x = 1.0, y = 0.0, z = 0.0, vx = 0.0, vy = 0.8, vz = 0.0, mass = 1.0},
    {x = -1.0, y = 0.0, z = 0.0, vx = 0.0, vy = -0.8, vz = 0.0, mass = 1.0},
    {x = 0.0, y = 1.2, z = 0.0, vx = -0.7, vy = 0.0, vz = 0.0, mass = 0.5},
    {x = 0.0, y = -1.2, z = 0.0, vx = 0.7, vy = 0.0, vz = 0.0, mass = 0.5},
  }
  local dt = 0.01
  for _ = 1, 400 do
    for i = 1, 5 do
      local bi = bodies[i]
      for j = i + 1, 5 do
        local bj = bodies[j]
        local dx = bj.x - bi.x
        local dy = bj.y - bi.y
        local dz = bj.z - bi.z
        local dist2 = dx * dx + dy * dy + dz * dz + 1e-9
        local inv_dist = 1.0 / math.sqrt(dist2)
        local inv_dist3 = inv_dist * inv_dist * inv_dist
        local force = inv_dist3 * dt
        local im = bi.mass * force
        local jm = bj.mass * force
        bi.vx = bi.vx + dx * jm
        bi.vy = bi.vy + dy * jm
        bi.vz = bi.vz + dz * jm
        bj.vx = bj.vx - dx * im
        bj.vy = bj.vy - dy * im
        bj.vz = bj.vz - dz * im
      end
    end
    for i = 1, 5 do
      local b = bodies[i]
      b.x = b.x + b.vx * dt
      b.y = b.y + b.vy * dt
      b.z = b.z + b.vz * dt
    end
  end
  local acc = 0
  for i = 1, 5 do
    local b = bodies[i]
    acc = acc ~ math.floor((b.x + b.y + b.z) * 1000000.0)
  end
  return mix_checksum(acc, seed)
end

local function work_native_loop(seed)
  local acc = seed
  for i = 0, 4000000 do
    acc = (acc + ((i * 3) ~ (i >> 2))) & 0xffffffffffffffff
  end
  return acc
end

local function work_particles_kinematics(seed)
  local particles = {}
  for i = 1, 6000 do
    particles[i] = {
      px = math.sin(i * 0.1),
      py = math.cos(i * 0.2),
      pz = math.sin(i * 0.3),
      vx = 0.01 * (i % 7),
      vy = 0.015 * (i % 11),
      vz = 0.02 * (i % 13),
    }
  end
  for _ = 1, 100 do
    for _, p in ipairs(particles) do
      p.vx = p.vx + (-p.px * 0.0003)
      p.vy = p.vy + (-p.py * 0.0002)
      p.vz = p.vz + (-p.pz * 0.0004)
      p.px = p.px + p.vx
      p.py = p.py + p.vy
      p.pz = p.pz + p.vz
    end
  end
  local acc = 0
  for _, p in ipairs(particles) do
    acc = acc ~ math.floor((p.px + p.py + p.pz) * 100000.0)
  end
  return mix_checksum(acc, seed)
end

local function work_primes_loop(seed)
  local limit = 50000 + (seed & 15)
  local is_prime = {}
  for i = 0, limit do is_prime[i] = true end
  is_prime[0] = false
  is_prime[1] = false
  local i = 2
  while i * i <= limit do
    if is_prime[i] then
      local j = i * i
      while j <= limit do
        is_prime[j] = false
        j = j + i
      end
    end
    i = i + 1
  end
  local acc = 0
  for n = 2, limit do
    if is_prime[n] then acc = acc + n end
  end
  return acc
end

local function sha256_bytes(input)
  local data = {}
  for i = 1, #input do
    data[#data + 1] = string.byte(input, i)
  end
  local bit_size = #data * 8
  data[#data + 1] = 0x80
  while (#data % 64) ~= 56 do
    data[#data + 1] = 0
  end
  for i = 7, 0, -1 do
    data[#data + 1] = (bit_size >> (i * 8)) & 0xff
  end

  local w = {}
  local h = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
  }

  for chunk = 1, #data, 64 do
    for i = 0, 15 do
      local idx = chunk + i * 4
      w[i] = ((data[idx] << 24) | (data[idx + 1] << 16) | (data[idx + 2] << 8) | data[idx + 3]) & 0xffffffff
    end
    for i = 16, 63 do
      local s0 = (rotr(w[i - 15], 7) ~ rotr(w[i - 15], 18) ~ (w[i - 15] >> 3)) & 0xffffffff
      local s1 = (rotr(w[i - 2], 17) ~ rotr(w[i - 2], 19) ~ (w[i - 2] >> 10)) & 0xffffffff
      w[i] = (w[i - 16] + s0 + w[i - 7] + s1) & 0xffffffff
    end
    local a, b, c, d, e, f, g, hh = h[1], h[2], h[3], h[4], h[5], h[6], h[7], h[8]
    for i = 0, 63 do
      local S1 = (rotr(e, 6) ~ rotr(e, 11) ~ rotr(e, 25)) & 0xffffffff
      local ch = (e & f) ~ ((~e) & g)
      local temp1 = (hh + S1 + ch + sha256_k[i + 1] + w[i]) & 0xffffffff
      local S0 = (rotr(a, 2) ~ rotr(a, 13) ~ rotr(a, 22)) & 0xffffffff
      local maj = (a & b) ~ (a & c) ~ (b & c)
      local temp2 = (S0 + maj) & 0xffffffff
      hh = g
      g = f
      f = e
      e = (d + temp1) & 0xffffffff
      d = c
      c = b
      b = a
      a = (temp1 + temp2) & 0xffffffff
    end
    h[1] = (h[1] + a) & 0xffffffff
    h[2] = (h[2] + b) & 0xffffffff
    h[3] = (h[3] + c) & 0xffffffff
    h[4] = (h[4] + d) & 0xffffffff
    h[5] = (h[5] + e) & 0xffffffff
    h[6] = (h[6] + f) & 0xffffffff
    h[7] = (h[7] + g) & 0xffffffff
    h[8] = (h[8] + hh) & 0xffffffff
  end

  local digest = {}
  for i = 1, 8 do
    local value = h[i]
    digest[#digest + 1] = (value >> 24) & 0xff
    digest[#digest + 1] = (value >> 16) & 0xff
    digest[#digest + 1] = (value >> 8) & 0xff
    digest[#digest + 1] = value & 0xff
  end
  return digest
end

local function work_sha256(seed)
  local acc = 0
  for i = 0, 1499 do
    local text = "sha256-benchmark-" .. i
    local digest = sha256_bytes(text)
    acc = acc ~ ((digest[1] << 56) & 0xffffffffffffffff)
    acc = acc ~ ((digest[9] << 40) & 0xffffffffffffffff)
    acc = acc ~ ((digest[17] << 24) & 0xffffffffffffffff)
    acc = acc ~ ((digest[25] << 8) & 0xffffffffffffffff)
  end
  return mix_checksum(acc, seed)
end

local function work_queen(seed)
  return solve_queen_impl(0, 11 + (seed & 1), 0, 0, 0)
end

local function work_sort(seed)
  local values = {}
  for i = 1, 2048 do
    values[i] = mix32((i + seed * 17 + 1234) & 0xffffffff)
  end
  table.sort(values)
  local acc = 0
  for i = 1, #values, 31 do
    acc = mix_checksum(acc, values[i])
  end
  return acc
end

local function spectral_a(i, j)
  local ij = i + j
  return 1.0 / (((ij * (ij + 1)) / 2) + i + 1)
end

local function spectral_multiply(u, out)
  local n = #u
  for i = 0, n - 1 do
    local sum = 0.0
    for j = 0, n - 1 do
      sum = sum + spectral_a(i, j) * u[j + 1]
    end
    out[i + 1] = sum
  end
end

local function spectral_multiply_transposed(u, out)
  local n = #u
  for i = 0, n - 1 do
    local sum = 0.0
    for j = 0, n - 1 do
      sum = sum + spectral_a(j, i) * u[j + 1]
    end
    out[i + 1] = sum
  end
end

local function work_spectral_norm(seed)
  local n = 120
  local u = {}
  local v = {}
  local tmp = {}
  for i = 1, n do
    u[i] = 1.0
    v[i] = 0.0
    tmp[i] = 0.0
  end
  for _ = 1, 10 do
    spectral_multiply(u, tmp)
    spectral_multiply_transposed(tmp, v)
    spectral_multiply(v, tmp)
    spectral_multiply_transposed(tmp, u)
  end
  local uv = 0.0
  local vv = 0.0
  for i = 1, n do
    uv = uv + u[i] * v[i]
    vv = vv + v[i] * v[i]
  end
  local ratio = math.sqrt(uv / vv)
  return mix_checksum(0, math.floor(ratio * 1000000.0))
end

local function work_string2float(seed)
  local acc = 0
  for i = 1, 12000 do
    local text = tostring(((i + seed) % 97) * 0.03125 + 10.0)
    local value = tonumber(text) or 0.0
    acc = mix_checksum(acc, math.floor(value * 100000.0) + #text)
  end
  return acc
end

local function checksum_tree(node)
  if node == nil then return 0 end
  return node.value + checksum_tree(node.left) + checksum_tree(node.right)
end

local function make_tree(depth, seed)
  local node = { value = seed }
  if depth > 0 then
    node.left = make_tree(depth - 1, seed * 2 + 1)
    node.right = make_tree(depth - 1, seed * 2 + 2)
  end
  return node
end

local function work_tree(seed)
  local tree = make_tree(13, 1 + seed)
  return checksum_tree(tree)
end

local function run_repeat(repeat_count, work)
  local acc = 0
  for i = 0, repeat_count - 1 do
    acc = mix_checksum(acc, work(i))
  end
  return acc
end

function benchmark_dictionary(repeat_count) return run_repeat(repeat_count, work_dictionary) end
function benchmark_exp_loop(repeat_count) return run_repeat(repeat_count, work_exp_loop) end
function benchmark_fibonacci_loop(repeat_count) return run_repeat(repeat_count, work_fibonacci_loop) end
function benchmark_fibonacci_recursive(repeat_count) return run_repeat(repeat_count, work_fibonacci_recursive) end
function benchmark_float2string(repeat_count) return run_repeat(repeat_count, work_float2string) end
function benchmark_mandelbrot(repeat_count) return run_repeat(repeat_count, work_mandelbrot) end
function benchmark_n_bodies(repeat_count) return run_repeat(repeat_count, work_n_bodies) end
function benchmark_native_loop(repeat_count) return run_repeat(repeat_count, work_native_loop) end
function benchmark_particles_kinematics(repeat_count) return run_repeat(repeat_count, work_particles_kinematics) end
function benchmark_primes_loop(repeat_count) return run_repeat(repeat_count, work_primes_loop) end
function benchmark_sha256(repeat_count) return run_repeat(repeat_count, work_sha256) end
function benchmark_queen(repeat_count) return run_repeat(repeat_count, work_queen) end
function benchmark_sort(repeat_count) return run_repeat(repeat_count, work_sort) end
function benchmark_spectral_norm(repeat_count) return run_repeat(repeat_count, work_spectral_norm) end
function benchmark_string2float(repeat_count) return run_repeat(repeat_count, work_string2float) end
function benchmark_tree(repeat_count) return run_repeat(repeat_count, work_tree) end
