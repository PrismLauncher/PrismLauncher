/* Copyright 2013-2017 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <hoedown/html.h>
#include <hoedown/document.h>
#include <QString>
#include <QByteArray>

/**
 * hoedown wrapper, because dealing with resource lifetime in C is stupid
 */
class HoeDown
{
public:
	class buffer
	{
	public:
		buffer(size_t unit = 4096)
		{
			buf = hoedown_buffer_new(unit);
		}
		~buffer()
		{
			hoedown_buffer_free(buf);
		}
		const char * cstr()
		{
			return hoedown_buffer_cstr(buf);
		}
		void put(QByteArray input)
		{
			hoedown_buffer_put(buf, (uint8_t *) input.data(), input.size());
		}
		const uint8_t * data() const
		{
			return buf->data;
		}
		size_t size() const
		{
			return buf->size;
		}
		hoedown_buffer * buf;
	} ib, ob;
	HoeDown()
	{
		renderer = hoedown_html_renderer_new((hoedown_html_flags) 0,0);
		document = hoedown_document_new(renderer, (hoedown_extensions) 0, 8);
	}
	~HoeDown()
	{
		hoedown_document_free(document);
		hoedown_html_renderer_free(renderer);
	}
	QString process(QByteArray input)
	{
		ib.put(input);
		hoedown_document_render(document, ob.buf, ib.data(), ib.size());
		return ob.cstr();
	}
private:
	hoedown_document * document;
	hoedown_renderer * renderer;
};
