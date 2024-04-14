#include "common/shard.hpp"

std::pair<Shard, Shard> split_shard(const Shard& shard) {
  assert(shard.lower != shard.upper);
  size_t lower = str_to_bucket(shard.lower), upper = str_to_bucket(shard.upper);
  uint64_t midpoint = lower + (upper - lower) / 2;
  // get new bounds for each shard (upper bound of lower shard, and lower bound
  // of upper shard)
  std::string lower_upper = bucket_to_str(midpoint, shard.granularity()),
              upper_lower = bucket_to_str(midpoint + 1, shard.granularity());
  return std::pair<Shard, Shard>{{shard.lower, lower_upper},
                                 {upper_lower, shard.upper}};
}

std::pair<Shard, Shard> split_shard(const Shard& shard, uint64_t at) {
  size_t lower = str_to_bucket(shard.lower), upper = str_to_bucket(shard.upper);
  assert(lower < upper);
  // get new bounds for each shard (upper bound of lower shard, and lower bound
  // of upper shard)
  std::string lower_upper = bucket_to_str(at, shard.granularity()),
              upper_lower = bucket_to_str(at + 1, shard.granularity());
  return std::pair<Shard, Shard>{{shard.lower, lower_upper},
                                 {upper_lower, shard.upper}};
}

std::pair<Shard, Shard> split_shard(const Shard& shard, const std::string& at,
                                    bool first) {
  // check that shard is larger than 1, and split location is within range
  assert(shard.lower != shard.upper);
  assert(shard.lower <= at && at <= shard.upper);
  size_t pos = str_to_bucket(at);
  // get new bounds for each shard (upper bound of lower shard, and lower bound
  // of upper shard)
  std::string lower_upper = bucket_to_str(pos - !first, shard.granularity()),
              upper_lower = bucket_to_str(pos + first, shard.granularity());
  return std::pair<Shard, Shard>{{shard.lower, lower_upper},
                                 {upper_lower, shard.upper}};
}

OverlapStatus get_overlap(const Shard& a, const Shard& b) {
  if (a.upper < b.lower || b.upper < a.lower) {
    /**
     * A: [-----]
     * B:         [-----]
     * Note: this also includes the flipped case (where B's keys are entirely
     * before A's)
     */
    return OverlapStatus::NO_OVERLAP;
  } else if (b.lower <= a.lower && a.upper <= b.upper) {
    /**
     * A:    [----]
     * B:  [--------]
     * Note: This also includes the case where the two shards are equal!
     */
    return OverlapStatus::COMPLETELY_CONTAINED;
  } else if (a.lower < b.lower && a.upper > b.upper) {
    /**
     * A: [-------]
     * B:   [---]
     */
    return OverlapStatus::COMPLETELY_CONTAINS;
  } else if (a.lower >= b.lower && a.upper > b.upper) {
    /**
     * A:    [-----]
     * B: [----]
     */
    return OverlapStatus::OVERLAP_START;
  } else if (a.lower < b.lower && a.upper <= b.upper) {
    /**
     * A: [-------]
     * B:    [------]
     */
    return OverlapStatus::OVERLAP_END;
  } else {
    throw std::runtime_error("bad case in get_overlap\n");
    return OverlapStatus::NO_OVERLAP;
  }
}

/* ==================================================*/
/* === INTERNALS: DO NOT MODIFY BELOW THIS LINE ===  */
/* ==================================================*/

// Sort shards in ascending order, by the lower bound.
void sort_shards(std::vector<Shard>& shards) {
  return std::sort(
      shards.begin(), shards.end(),
      [](const Shard& a, const Shard& b) { return a.lower < b.lower; });
}

bool is_valid(const std::string& s) {
  for (auto&& c : s) {
    if (std::find(VALID_CHARS.cbegin(), VALID_CHARS.cend(), std::toupper(c)) ==
        VALID_CHARS.cend())
      return false;
  }
  return true;
}

std::string bucket_to_str(size_t bucket, size_t granularity) {
  // Get the bucket's indices at each granularity within VALID_CHARS to
  // construct the string
  std::array<size_t, MAX_GRANULARITY> indices{};
  size_t n_left = bucket;

  // Use reverse order through GRANULARITY_OPTS, as we want to find the largest
  // bucket at the current index before moving onto the next one.
  //
  // For example, with bucket = 1650, granularity = 3,
  // we would iterate from GRANULARITY_OPTS[2 -> 0], or { 1296, 36, 1 }.
  // The 1650th bucket would have indices { 1, 9, 30 }.
  // - At index 0, we have 1650 / 1296 = 1, with 354 left.
  // - At index 1, we have 354 / 36 = 9, with 30 left.
  // - At index 2, we have 30 / 1 = 30.
  for (size_t i = 0; i < granularity; i++) {
    size_t index = n_left / GRANULARITY_OPTS[granularity - i - 1];
    indices[i] = index;
    n_left = n_left % GRANULARITY_OPTS[granularity - i - 1];
  }

  // now, construct res from given indices
  // continuing with the example, given indices {1, 9, 30}, res = "19U"
  std::string res(granularity, '\0');
  for (size_t i = 0; i < granularity; i++) {
    res[i] = VALID_CHARS[indices[i]];
  }
  return res;
}

size_t str_to_bucket(const std::string& s) {
  /*
  First, calculate the granularity of s.
  This tells us the number of keys in our keyspace.
  We divide these keys among 36 buckets -- that way, each character in
  VALID_CHARS corresponds to a bucket containing 36^(granularity - 1) keys. For
  example, if granularity = 3, the keyspace is [000, ZZZ], the total number of
  keys is 36^3, and each bucket maps to 36^2 keys.
  */
  size_t granularity = s.size();

  /*
  Example: given s = "ABB", bucket = 13367:
  For each character in the string, find the appropriate bucket by getting the
  index of the character in VALID_CHARS. Then, multiply that bucket by the
  number of keys in that bucket.
  - At character 'A' (index 10), granularity(ABB) = 3, so bucket += 10 * 36^2.
  - At character 'B' (index 11), granularity(BB) = 2, so bucket += 11 * 36^1
  - At character 'B' (index 11), granularity(B) = 1, so bucket += 11 * 36^0
  */
  size_t bucket = 0;
  for (size_t i = 0; i < granularity; i++) {
    // Find the character's index within VALID_CHARS, or throw an error.
    auto pos =
        std::find(VALID_CHARS.cbegin(), VALID_CHARS.cend(), std::toupper(s[i]));
    assert(pos != VALID_CHARS.cend());
    size_t idx = pos - VALID_CHARS.cbegin();
    bucket += idx * GRANULARITY_OPTS[granularity - i - 1];
  }
  return bucket;
}

std::vector<Shard> split_into(size_t n_shards) {
  assert(n_shards > 0);

  // Determine the shard granularity required to satisfy this number of shards
  size_t n_buckets = 0, granularity = 0;
  for (int i = 1; i <= MAX_GRANULARITY; i++) {
    if (auto opt = GRANULARITY_OPTS[i]; n_shards <= opt) {
      n_buckets = opt;
      granularity = i;
      break;
    }
  }

  // Get size of each shard
  size_t shard_size = n_buckets / n_shards;
  size_t remainder = n_buckets % n_shards;
  std::vector<size_t> shard_sizes(n_shards, shard_size);
  // uneven splitting -- distribute evenly among first [0..remainder] shards
  for (size_t i = 0; i < remainder; i++) {
    shard_sizes[i] += 1;
  }

  // For each shard, get its lower and upper bounds, then convert into string
  // representation
  std::vector<Shard> shards(n_shards);
  size_t lower = 0;
  for (size_t i = 0; i < n_shards; i++) {
    size_t upper = lower + shard_sizes[i] - 1;
    std::string lower_bucket = bucket_to_str(lower, granularity),
                upper_bucket = bucket_to_str(upper, granularity);
    shards[i] = {lower_bucket, upper_bucket};
    lower += shard_sizes[i];
  }
  return shards;

  /*
  Example: n_shards = 2
  Then n_buckets = 36, granularity = 1
  shard_size = 36 / 2 = 18, remainder = 0
  shard_sizes = [18. 18]
  shards loop
  (first iteration)
      lower = 0, upper = 17
      lower_bucket = bucket_to_str(0, 1) = 0, upper_bucket = bucket_to_str(17,
  1) = G (second iteration) lower = 18, upper = 35 lower_bucket =
  bucket_to_str(18, 1) = H, upper_bucket = bucket_to_str(35, 1) = Z shards =
  [{0, G}, {H, Z}]
  */

  /*
  Example: n_shards = 1000
  Then n_buckets = 1296 (36^2), granularity = 2
  shard_size = 1296 / 1000 = 1, remainder = 296
  shard_sizes = [2, 2, .... 1, 1 ] (vector of length 1000, with 2 296 times,
  then 1 704 times) shards loop (first iteration) lower = 0, upper = 1
      lower_bucket = bucket_to_str(0, 2) = 00, upper_bucket = bucket_to_str(1,
  2) = 01 (second iteration) lower = 2, upper = 3 lower_bucket =
  bucket_to_str(2, 2) = 02, upper_bucket = bucket_to_str(3, 2) = 03
  ...
  shards = [{00, 01}, {02, 03}, ..., {FD, FE}, {FF, FF}, ... {ZZ, ZZ}]
  note that the shard size is 2 296 times, then switches to size 1 from {FF}
  onward b/c bucket_to_str(2 * 296, 2) = FF
  */
}
