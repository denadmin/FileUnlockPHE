/**************************************************************************
 *  FileUnlock plug-in for FAR 3.0                                        *
 *  Copyright (C) 2010-2014 by Artem Senichev <artemsen@gmail.com>        *
 *  https://sourceforge.net/projects/farplugs/                            *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#pragma once

#include "common.h"


class progress
{
public:
	/**
	 * Constructor.
	 * \param max_value maximal progress value
	 */
	progress(const uint32_t max_value);

	~progress();

	/**
	 * Hide progress window.
	 */
	void hide();

	/**
	 * Set progress value.
	 * \param progr new progress value
	 * \param found found items count
	 */
	void update(const uint32_t progr, const uint16_t found);

	/**
	 * Check for abort request.
	 * \return true if user requested abort
	 */
	static bool aborted();

private:
	bool			_visible;	///< Visible flag
	const wchar_t*	_title;		///< Window title
	uint32_t		_max_value;	///< Maximum progress value
	wstring			_bar;		///< Progress bar
};
