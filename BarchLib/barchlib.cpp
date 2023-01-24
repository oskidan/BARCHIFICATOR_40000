#include "barchlib.hpp"

#include <algorithm> // for std::find_if
#include <cstring>   // for std::memset, std::memcpy
#include <limits>    // for std::numeric_limits
#include <new>       // for std::bad_alloc
#include <utility>   // for std::move

//******************************************************************************

namespace BarchLib::inline v1::Internal {

[[noreturn]] inline void throwInvalidX(std::size_t value) {
  throw InvalidCoordinate{InvalidCoordinate::X, value};
}

[[noreturn]] inline void throwInvalidY(std::size_t value) {
  throw InvalidCoordinate{InvalidCoordinate::Y, value};
}

[[nodiscard]] inline bool isEmpty(const ImmutablePixels pixels) {
  auto const end = pixels.end();
  return end == std::find_if(pixels.begin(), end,
                             [](const Pixel pixel) { return pixel != White; });
}

constexpr PixelBlock WhiteBlock = combine(White, White, White, White);
constexpr PixelBlock BlackBlock = combine(Black, Black, Black, Black);

void Encoder::encode(const ImmutablePixels pixels) {
  std::size_t pixelCount = pixels.size();
  std::size_t pixelIndex = 0;
  while (pixelCount >= 4) {
    write(combine(pixels[pixelIndex + 0], pixels[pixelIndex + 1],
                  pixels[pixelIndex + 2], pixels[pixelIndex + 3]));
    pixelCount -= 4;
    pixelIndex += 4;
  }
  // Handle remaining pixels. This happens when pixels are not multiple of four.
  switch (pixelCount) {
  default:
    // Nothing to do.
    break;
  case 1:
    write(combine(pixels[pixelIndex + 0], Black, Black, Black));
    break;
  case 2:
    write(
        combine(pixels[pixelIndex + 0], pixels[pixelIndex + 1], Black, Black));
    break;
  case 3:
    write(combine(pixels[pixelIndex + 0], pixels[pixelIndex + 1],
                  pixels[pixelIndex + 2], Black));
    break;
  }
}

// clang-format off
inline void Encoder::write0() { m_output->clear(m_index++); }
inline void Encoder::write1() { m_output->set  (m_index++); }
// clang-format on

void Encoder::write(const PixelBlock block) {
  if (block == WhiteBlock) {
    write0();
    return;
  }
  if (block == BlackBlock) {
    write1();
    write0();
    return;
  }

  write1();
  write1();
  for (PixelBlock bitMask = 0x80'00'00'00U; bitMask; bitMask >>= 1) {
    (this->*Write[!!(block & bitMask)])();
  }
}

void Decoder::decode(const MutablePixels pixels) {
  std::size_t pixelCount = pixels.size();
  std::size_t pixelIndex = 0;
  while (pixelCount >= 4) {
    std::array<Pixel, 4> block = split(read());
    pixels[pixelIndex + 0] = block[0];
    pixels[pixelIndex + 1] = block[1];
    pixels[pixelIndex + 2] = block[2];
    pixels[pixelIndex + 3] = block[3];
    pixelCount -= 4;
    pixelIndex += 4;
  }
  switch (pixelCount) {
  default:
    // Nothing to do.
    break;
  case 1: {
    std::array<Pixel, 4> block = split(read());
    pixels[pixelIndex + 0] = block[0];
  } break;
  case 2: {
    std::array<Pixel, 4> block = split(read());
    pixels[pixelIndex + 0] = block[0];
    pixels[pixelIndex + 1] = block[1];
  } break;
  case 3: {
    std::array<Pixel, 4> block = split(read());
    pixels[pixelIndex + 0] = block[0];
    pixels[pixelIndex + 1] = block[1];
    pixels[pixelIndex + 2] = block[2];
  } break;
  } // switch (pixelCount)
}

bool Decoder::readBit() { return m_input->test(m_index++); }

PixelBlock Decoder::read() {
  if (!readBit()) {
    // Bit pattern: 0
    return WhiteBlock;
  }
  if (!readBit()) {
    // Bit pattern: 10
    return BlackBlock;
  }
  // Bit pattern: 11
  PixelBlock result = 0;
  for (PixelBlock bitMask = 0x80'00'00'00U; bitMask; bitMask >>= 1) {
    if (readBit()) { result |= bitMask; }
  }
  return result;
}

} // namespace BarchLib::inline v1::Internal

//******************************************************************************

namespace BarchLib::inline v1 {

const char *InvalidSize::what() const noexcept {
  switch (m_reason) {
  case TooSmall:
    return "An error occured while constructing the bitmap. "
           "The requested size is too small. "
           "The smallest bitmap size is 1x1.";
  case TooLarge:
    return "An error occurred while constructing the bitmap. "
           "The requested size is too large. ";
  default:
    return "An error occurred while constructing the bitmap. "
           "The requested size is invalid. ";
  }
}

const char *InvalidCoordinate::what() const noexcept {
  switch (m_kind) {
  case X:
    return "An error occurred while accessing the pixel data. "
           "X coordinate is out of bounds.";
  case Y:
    return "An error occurred while accessing the pixel data. "
           "Y coordinate is out of bounds.";
  default:
    return "An error occurred while accessing the pixel data. "
           "Coordinate is out of bounds.";
  }
}

namespace Internal {

BitmapSize::BitmapSize(const std::size_t width, const std::size_t height)
    : m_width{width}, m_height{height} {
  if (width == 0 || height == 0) {
    throw InvalidSize{width, height, InvalidSize::TooSmall};
  }
  // The maximum height that won't overflow with the given width.
  if (std::size_t maximumHeight =
          std::numeric_limits<std::size_t>::max() / width;
      height > maximumHeight) {
    throw InvalidSize{width, height, InvalidSize::TooLarge};
  }
}

} // namespace Internal

Bitmap::Bitmap(const std::size_t width, const std::size_t height,
               const Pixel background)
    : m_size{width, height} {
  try {
    std::size_t dataSize = width * height;
    m_data = std::make_unique<Pixel[]>(dataSize);
    // Could've used fill_n here, but it's slower (especially in debug).
    std::memset(m_data.get(), background, dataSize);
  } catch (std::bad_alloc &) {
    throw InvalidSize{width, height, InvalidSize::TooLarge};
  }
}

Bitmap::Bitmap(const Bitmap &other) : m_size{other.m_size} {
  std::size_t const pixelCount = other.pixelCount();
  m_data = std::make_unique<Pixel[]>(pixelCount);
  std::memcpy(m_data.get(), other.data(), pixelCount);
}

Bitmap::Bitmap(Bitmap &&other) noexcept
    : m_size{other.m_size}, m_data{std::move(other.m_data)} {}

MutablePixels Bitmap::rowAt(const std::size_t y) {
  if (y >= height()) { Internal::throwInvalidY(y); };
  return {data() + y * width(), width()};
}

ImmutablePixels Bitmap::rowAt(const std::size_t y) const {
  if (y >= height()) { Internal::throwInvalidY(y); }
  return {data() + y * width(), width()};
}

Pixel &Bitmap::pixelAt(const std::size_t x, const std::size_t y) {
  if (x >= width()) { Internal::throwInvalidX(x); }
  if (y >= height()) { Internal::throwInvalidY(y); }
  return data()[y * width() + x];
}

Pixel const &Bitmap::pixelAt(const std::size_t x, const std::size_t y) const {
  if (x >= width()) { Internal::throwInvalidX(x); }
  if (y >= height()) { Internal::throwInvalidY(y); }
  return data()[y * width() + x];
}

namespace Internal {

BitSet::BitSet(const std::size_t bitCount) {
  const std::size_t maxPadding = bitsPer<Word> - std::size_t{1};
  // Align the number of bits, so we get a whole number of words.
  const std::size_t alignedBitCount = (bitCount + maxPadding) & ~maxPadding;
  const std::size_t wordCount = alignedBitCount / bitsPer<Word>;
  m_words.resize(wordCount);
}

bool BitSet::test(const std::size_t bitIndex) const {
  auto const [wordIndex, bitMask] = toWord(bitIndex);
  if (wordIndex >= m_words.size()) {
    // Out of range bits are considered to be off.
    return false;
  }
  return static_cast<bool>(m_words[wordIndex] & bitMask);
}

void BitSet::set(const std::size_t bitIndex) {
  auto const [wordIndex, bitMask] = toWord(bitIndex);
  if (wordIndex >= m_words.size()) {
    m_words.resize(wordIndex + std::size_t{1});
  }
  m_words[wordIndex] |= bitMask;
}

void BitSet::clear(const std::size_t bitIndex) {
  auto const [wordIndex, bitMask] = toWord(bitIndex);
  if (wordIndex >= m_words.size()) {
    // Out of range bits are considered to be off.
    return;
  }
  m_words[wordIndex] &= ~bitMask;
}

std::pair<std::size_t /* wordIndex */, Word /* bitMask */>
BitSet::toWord(const std::size_t bitIndex) {
  // NOTE: assumes that only 32-bit and 64-bit targets are supported.
  if constexpr (sizeof(Word) == sizeof(std::uint64_t)) {
    return {bitIndex / bitsPer<Word>,
            Word{0x80'00'00'00'00'00'00'00} >> (bitIndex % bitsPer<Word>)};
  } else {
    return {bitIndex / bitsPer<Word>,
            Word{0x80'00'00'00} >> (bitIndex % bitsPer<Word>)};
  }
}

} // namespace Internal

CompressedBitmap::CompressedBitmap(const std::size_t width,
                                   const std::size_t height)
    : m_size{width, height}, m_rowLookupTable{height} {}

bool CompressedBitmap::isEmptyRowAt(const std::size_t y) const {
  if (y >= height()) { Internal::throwInvalidY(y); }
  return !m_rowLookupTable.test(y);
}

CompressedBitmap compress(const Bitmap &sourceBitmap,
                          const ProgressHandler progress) {
  const std::size_t width = sourceBitmap.width();
  const std::size_t height = sourceBitmap.height();
  CompressedBitmap result{width, height};
  Internal::Encoder rowEncoder{result.m_pixelData};
  for (std::size_t y = 0; y < height; ++y) {
    progress(y, height);
    const ImmutablePixels currentRow = sourceBitmap.rowAt(y);
    if (Internal::isEmpty(currentRow)) {
      // Empty rows are skipped. The corresponding entry in the lookup table is
      // set to 0 anyways.
      continue;
    }
    result.m_rowLookupTable.set(y);
    rowEncoder.encode(currentRow);
  }
  progress(height, height);
  return result;
}

Bitmap uncompress(const CompressedBitmap &sourceBitmap,
                  const ProgressHandler progress) {
  const std::size_t width = sourceBitmap.width();
  const std::size_t height = sourceBitmap.height();
  Bitmap result{width, height};
  Internal::Decoder rowDecoder{sourceBitmap.m_pixelData};
  for (std::size_t y = 0; y < height; ++y) {
    progress(y, height);
    if (sourceBitmap.m_rowLookupTable.test(y)) {
      rowDecoder.decode(result.rowAt(y));
    }
  }
  progress(height, height);
  return result;
}

} // namespace BarchLib::inline v1

//******************************************************************************
