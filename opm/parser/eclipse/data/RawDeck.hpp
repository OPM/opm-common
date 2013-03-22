/* 
 * File:   RawDeck.hpp
 * Author: kflik
 *
 * Created on March 21, 2013, 2:02 PM
 */

#ifndef RAWDECK_HPP
#define	RAWDECK_HPP
#include <list>
#include "RawKeyword.hpp"

namespace Opm {

    class RawDeck {
    public:
        RawDeck();
        void addKeyword(const RawKeyword& keyword);
        unsigned int getNumberOfKeywords();
        virtual ~RawDeck();
    private:
        std::list<RawKeyword> m_keywords;
    };
}

#endif	/* RAWDECK_HPP */

