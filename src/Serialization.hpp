// MIT License
// 
// Copyright (c) 2024 Mihail Mladenov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Bezier.hpp"
#include "Rendering.hpp"
#include "Image.hpp"
#include "File.hpp"
#include "Webp.hpp"
#include "Utilities.hpp"
#include "Algorithm.hpp"
#include "Concepts.hpp"

namespace PA
{
	template <typename TF>
	inline auto SerializeToSVG
	(
		Span<const QuadraticBezier<TF, 2>> normalizedCoords,
		Span<const TF> widths,
		Span<const TF> pigments,
		U32 width,
		U32 height,
		StrView outFile = "out.svg"sv
	);

	inline auto SerializeToWebP(RawCPUImage& hdrSurface, StrView outFile = "out.webp");

	template <typename TF>
	inline auto SerializeToFrames
	(
		Span<const QuadraticBezier<TF, 2>> normalizedCoords,
		Span<const TF> widths,
		Span<const TF> pigments,
		U32 width,
		U32 height,
		StrView outFolder = "out"sv
	);

	template <typename T>
		requires CIsArithmetic<T>
	inline auto Serialize(Array<Byte>& outBuffer, T in) -> B;

	template <typename T, U32 Dim>
	inline auto Serialize(Array<Byte>& outBuffer, Vector<T, Dim> in) -> B;

	template <typename T, U32 Dim>
	inline auto Serialize(Array<Byte>& outBuffer, QuadraticBezier<T, Dim> in) -> B;

	template <typename T>
	inline auto Serialize(Array<Byte>& outBuffer, Span<const T> in) -> B;

	template <typename T>
	inline auto Serialize(Array<Byte>& outBuffer, const Array<T>& in) -> B;

	template<typename T>
		requires CIsArithmetic<T>
	auto Serialize(Span<Byte>& outBuffer, T in) -> B;

	template<typename T>
	auto Serialize(Span<Byte>& outBuffer, Span<const T> in) -> B;

	inline auto Serialize(Array<Byte>& outBuffer, Fragment frag) -> B;

	template <typename T>
		requires CIsArithmetic<T>
	inline auto Deserialize(Span<const Byte>& inBuffer, T& out) -> B;

	template <typename T, U32 Dim>
	inline auto Deserialize(Span<const Byte>& inBuffer, Vector<T, Dim>& out) -> B;

	template <typename T, U32 Dim>
	inline auto Deserialize(Span<const Byte>& inBuffer, QuadraticBezier<T, Dim>& out) -> B;

	template <typename T>
	inline auto Deserialize(Span<const Byte>& inBuffer, Span<T> outBuffer) -> B;

	template <typename T>
	inline auto Deserialize(Span<const Byte>& inBuffer, Array<T>& outBuffer) -> B;

	inline auto Deserialize(Span<const Byte>& inBuffer, Fragment& frag) -> B;
}

namespace PA
{
	template<typename TF>
	auto SerializeToSVG
	(
		Span<const QuadraticBezier<TF, 2>> normalizedCoords,
		Span<const TF> widths,
		Span<const TF> pigments,
		U32 width,
		U32 height,
		StrView outFile
	)
	{
		Str svg = "<svg xmlns = \"http://www.w3.org/2000/svg\" width =\"";
		svg += ToString(width);
		svg += "\" height=\"" + ToString(height) + "\"";
		svg += " viewBox=\"0 0 " + ToString(width) + " ";
		svg += ToString(height) + "\">\n";
		svg += "<style>path { mix-blend-mode: darken; }</style>\n";

		for (auto i = 0; i < normalizedCoords.size(); ++i)
		{
			const auto& q = normalizedCoords[i];
			const auto& w = widths[i];
			const auto pigment = ClampedU8(255u - 255u * pigments[i]);
			const auto color = ColorU32(255u, pigment, pigment, pigment);
			const auto hexColor = '#' + Format("{:08x}", color.packed);

			auto qp0 = ToSurfaceCoordinates(q.p0, width, height);
			auto qp1 = ToSurfaceCoordinates(q.p1, width, height);
			auto qp2 = ToSurfaceCoordinates(q.p2, width, height);

			// Get the control points of the corresponding cubic curve.
			// The start and the end point are the same as the ones of
			// the quadratic curve.
			auto cp1 = qp0 + TF(2) / TF(3) * (qp1 - qp0);
			auto cp2 = qp2 + TF(2) / TF(3) * (qp1 - qp2);

			svg += "<path fill=\"none\" ";
			svg += "stroke=\"" + hexColor + "\" ";
			svg += "stroke-width=\"" + ToString(w) + "\" ";
			svg += "d=\"";
			svg += "M " + ToString(qp0[0]) + " " + ToString(qp0[1]) + " ";
			svg += "C " + ToString(cp1[0]) + " " + ToString(cp1[1]) + " ";
			svg += ToString(cp2[0]) + " " + ToString(cp2[1]) + " ";
			svg += ToString(qp2[0]) + " " + ToString(qp2[1]) + " ";
			svg += "\"/>\n";
		}

		svg += "</svg>";
		WriteWholeFile(outFile, { (const Byte*)svg.data(), svg.size() });
	}


	inline auto SerializeToWebP(RawCPUImage& hdrSurface, StrView outFile)
	{
		auto rgba8Surface = A32FloatToRGBA8Linear(hdrSurface);
		auto webpEncoded = EncodeWebP(rgba8Surface, 70);
		WriteWholeFile(outFile, Span<const Byte>(webpEncoded.data(), webpEncoded.size()));
	}


	template<typename TF>
	auto SerializeToFrames
	(
		Span<const QuadraticBezier<TF, 2>> normalizedCoords,
		Span<const TF> widths,
		Span<const TF> pigments,
		U32 width,
		U32 height,
		StrView outFolder
	)
	{
		Log("Serializing to frames");
		RemoveDirectoryRecursive(outFolder);
		CreateDirectory(outFolder);

		auto frameCount = 0u;
		auto seq = GenerateSequence(U32(0), U32(normalizedCoords.size()));

		Sort
		(
			seq, 
			[&](U32 i0, U32 i1)
			{ 
				const auto& c0 = normalizedCoords[i0];
				const auto w0 = widths[i0];
				const auto p0 = pigments[i0];
				const auto& c1 = normalizedCoords[i1];
				const auto w1 = widths[i1];
				const auto p1 = pigments[i1];
				auto c0LenApprox = Distance(c0.p0, c0.p1) + Distance(c0.p1, c0.p2);
				auto c1LenApprox = Distance(c1.p0, c1.p1) + Distance(c1.p1, c1.p2);

				if (w0 > w1)
				{
					return true;
				}
				else if (p0 < p1)
				{
					return true;
				}
				else if (c0LenApprox > c1LenApprox)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
		);

		RawCPUImage surface(width, height, EFormat::A32Float, true);
		Array<Fragment> fragments;
		surface.Clear(TF(1));

		static constexpr U32 logAfterFrames = 128;
		U32 curvesPerFrame = 0;
		for (auto i = 0u; i < seq.size(); ++i)
		{
			auto idx = seq[i];
			auto& curve = normalizedCoords[idx];
			auto lengthApprox = Distance(curve.p0, curve.p1) + Distance(curve.p1, curve.p2);
			auto strokeSegmentation = U32(TF(5) * lengthApprox);
			auto multiCurvesPerFrame = (lengthApprox < TF(0.05))? true : false;

			for (auto s = 1u; s < strokeSegmentation; ++s)
			{
				auto splitPoint = TF(s) / strokeSegmentation;
				auto currentCurve = curve.Split(splitPoint).first;

				RasterizeToFragments(currentCurve, fragments, width, height, pigments[idx], widths[idx]);
				PutFragmentsOnHDRSurface(fragments, surface);
				SerializeToWebP(surface, (Path(outFolder) / Format("frame{:06d}.webp", frameCount)).string());
				RemoveFragmentsFromHDRSurface(fragments, surface);
				frameCount++;
			}

			RasterizeToFragments(curve, fragments, width, height, pigments[idx], widths[idx]);
			PutFragmentsOnHDRSurface(fragments, surface);

			if (multiCurvesPerFrame && curvesPerFrame < 4 && i != seq.size() - 1)
			{
				SerializeToWebP(surface, (Path(outFolder) / Format("frame{:06d}.webp", frameCount)).string());
				curvesPerFrame = 0;
			}
			else
			{
				curvesPerFrame++;
			}

			frameCount++;
			if (i % logAfterFrames == 0)
			{
				auto progress = F32(i) / seq.size() * 100;
				Log(Format("Progress: {:3.2f}%", progress));
			}
		}
	}


	template<typename T>
		requires CIsArithmetic<T>
	auto Serialize(Array<Byte>& outBuffer, T in) -> B
	{
		auto oldSize = outBuffer.size();
		outBuffer.resize(oldSize + sizeof(T));
		in = FromLE(in);
		MemCopy(Span<const T>((const T*)&in, 1), outBuffer.data() + oldSize);
		return true;
	}


	template<typename T, U32 Dim>
	auto Serialize(Array<Byte>& outBuffer, Vector<T, Dim> in) -> B
	{
		return Serialize(outBuffer, Span<const T>(in.data.data(), Dim));
	}

	template<typename T, U32 Dim>
	auto Serialize(Array<Byte>& outBuffer, QuadraticBezier<T, Dim> in) -> B
	{
		using Vec = Vector<T, Dim>;
		return Serialize(outBuffer, Span<const Vec>((const Vec*)in.points.data(), 3));
	}


	template<typename T>
	auto Serialize(Array<Byte>& outBuffer, Span<const T> in) -> B
	{
		Serialize(outBuffer, (U32)in.size());
		for (const auto& x : in)
		{
			Serialize(outBuffer, x);
		}
		return true;
	}

	template<typename T>
	auto Serialize(Array<Byte>& outBuffer, const Array<T>& in) -> B
	{
		Serialize(outBuffer, (U32)in.size());
		for (const auto& x : in)
		{
			Serialize(outBuffer, x);
		}
		return true;
	}


	template<typename T>
		requires CIsArithmetic<T>
	auto Serialize(Span<Byte>& outBuffer, T in) -> B
	{
		if (outBuffer.size() < sizeof(T))
		{
			return false;
		}

		in = FromLE(in);
		MemCopy(Span<const T>(&in, 1), outBuffer.data());

		outBuffer = outBuffer.subspan(sizeof(T));
		return true;
	}


	template<typename T>
	auto Serialize(Span<Byte>& outBuffer, Span<const T> in) -> B
	{
		if (outBuffer.size() < sizeof(T) * in.size() + sizeof(U32))
		{
			return false;
		}

		Serialize(outBuffer, (U32)in.size());
		for (auto& x : in)
		{
			Serialize(outBuffer, x);
		}

		return true;
	}


	inline auto PA::Serialize(Array<Byte>& outBuffer, Fragment frag) -> B
	{
		return Serialize(outBuffer, frag.idx) && Serialize(outBuffer, frag.value);
	}


	template<typename T>
		requires CIsArithmetic<T>
	auto Deserialize(Span<const Byte>& inBuffer, T& out) -> B
	{
		if (inBuffer.size() < sizeof(T))
		{
			return false;
		}
		MemCopy(inBuffer.subspan(0, sizeof(T)), &out);
		out = FromLE(out);
		inBuffer = inBuffer.subspan(sizeof(T));
		return true;
	}


	template<typename T, U32 Dim>
	auto Deserialize(Span<const Byte>& inBuffer, Vector<T, Dim>& out) -> B
	{
		return Deserialize(inBuffer, Span<T>(out.data.data(), Dim));
	}


	template<typename T, U32 Dim>
	auto Deserialize(Span<const Byte>& inBuffer, QuadraticBezier<T, Dim>& out) -> B
	{
		using Vec = Vector<T, Dim>;
		return Deserialize(inBuffer, Span<Vec>((Vec*)out.points.data(), 3));
	}


	template<typename T>
	auto Deserialize(Span<const Byte>& inBuffer, Span<T> outBuffer) -> B
	{
		if (inBuffer.size() < sizeof(U32))
		{
			return false;
		}

		U32 arraySize;
		Deserialize(inBuffer, arraySize);

		if (inBuffer.size() < arraySize)
		{
			return false;
		}

		for (auto i = 0u; i < arraySize; ++i)
		{
			Deserialize(inBuffer, outBuffer[i]);
		}

		return true;
	}


	template<typename T>
	auto Deserialize(Span<const Byte>& inBuffer, Array<T>& outBuffer) -> B
	{
		if (inBuffer.size() < sizeof(U32))
		{
			return false;
		}

		U32 arraySize;
		Deserialize(inBuffer, arraySize);


		if (inBuffer.size() < arraySize)
		{
			return false;
		}

		auto oldSize = outBuffer.size();
		outBuffer.resize(oldSize + arraySize);

		for (auto i = 0; i < arraySize; ++i)
		{
			Deserialize(inBuffer, outBuffer[oldSize + i]);
		}

		return true;
	}

	inline auto PA::Deserialize(Span<const Byte>& inBuffer, Fragment& frag) -> B
	{
		return Deserialize(inBuffer, frag.idx) && Deserialize(inBuffer, frag.value);
	}
}