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
#include "Image.hpp"
#include "File.hpp"
#include "Utilities.hpp"

namespace PA
{
	template <typename TF>
	inline auto SerializeToSVG(Span<const QuadraticBezier<TF, 2>> normalizedCoords, TF width, TF height, StrView outFile = "out.svg"sv);

}

namespace PA
{
	template<typename TF>
	auto SerializeToSVG(Span<const QuadraticBezier<TF, 2>> normalizedCoords, TF width, TF height, StrView outFile)
	{
		Str svg = "<svg xmlns = \"http://www.w3.org/2000/svg\" width =\"";
		svg += ToString(width);
		svg += "\" height=\"" + ToString(height) + "\"";
		svg += " viewBox=\"0 0 " + ToString(width) + " ";
		svg += ToString(height) + "\">\n";
		svg += "<path stroke=\"#010101\" stroke-width=\"0.5\" d=\"";
		for (auto& q : normalizedCoords)
		{
			auto qp0 = ToSurfaceCoordinates(q.p0, width, height);
			auto qp1 = ToSurfaceCoordinates(q.p1, width, height);
			auto qp2 = ToSurfaceCoordinates(q.p2, width, height);

			// Get the control points of the corresponding cubic curve.
			// The start and the end point are the same as the ones of
			// the quadratic curve.
			auto cp1 = qp0 + TF(2) / TF(3) * (qp1 - qp0);
			auto cp2 = qp2 + TF(2) / TF(3) * (qp1 - qp2);

			svg += "M " + ToString(qp0[0]) + " " + ToString(qp0[1]) + " ";
			svg += "C " + ToString(cp1[0]) + " " + ToString(cp1[1]) + " ";
			svg += ToString(cp2[0]) + " " + ToString(cp2[1]) + " ";
			svg += ToString(qp2[0]) + " " + ToString(qp2[1]) + " ";
		}
		svg += "\"/>\n";
		svg += "</svg>";
		WriteWholeFile(outFile, { (const Byte*)svg.data(), svg.size() });
	}

}