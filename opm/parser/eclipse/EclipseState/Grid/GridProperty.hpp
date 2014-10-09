/*
  Copyright 2014 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ECLIPSE_GRIDPROPERTY_HPP_
#define ECLIPSE_GRIDPROPERTY_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <boost/lexical_cast.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridPropertyInitializers.hpp>

/*
  This class implemenents a class representing properties which are
  define over an ECLIPSE grid, i.e. with one value for each logical
  cartesian cell in the grid.

  The class is implemented as a thin wrapper around std::vector<T>;
  where the most relevant specialisations of T are 'int' and 'float'.
*/




namespace Opm {

template <class ValueType>
class GridPropertyBasePostProcessor
{
protected:
    GridPropertyBasePostProcessor() { }

public:
    virtual void apply(std::vector<ValueType>& values) const = 0;
};




template <class DataType>
class GridPropertySupportedKeywordInfo
{
public:
    typedef GridPropertyBaseInitializer<DataType> Initializer;
    typedef GridPropertyBasePostProcessor<DataType> PostProcessor;

    GridPropertySupportedKeywordInfo()
    {}

    GridPropertySupportedKeywordInfo(const std::string& name,
                                     std::shared_ptr<const Initializer> initializer,
                                     std::shared_ptr<const PostProcessor> postProcessor,
                                     const std::string& dimString)
        : m_keywordName(name),
          m_initializer(initializer),
          m_postProcessor(postProcessor),
          m_dimensionString(dimString)
    {}


    GridPropertySupportedKeywordInfo(const std::string& name,
                                     std::shared_ptr<const Initializer> initializer,
                                     const std::string& dimString)
        : m_keywordName(name),
          m_initializer(initializer),
          m_dimensionString(dimString)
    {}

    // this is a convenience constructor which can be used if the default value for the
    // grid property is just a constant...
    GridPropertySupportedKeywordInfo(const std::string& name,
                                     const DataType defaultValue,
                                     const std::string& dimString)
        : m_keywordName(name),
          m_initializer(new Opm::GridPropertyConstantInitializer<DataType>(defaultValue)),
          m_dimensionString(dimString)
    {}
    
    GridPropertySupportedKeywordInfo(const std::string& name,
                                     const DataType defaultValue,
                                     std::shared_ptr<const PostProcessor> postProcessor,
                                     const std::string& dimString)
        : m_keywordName(name),
          m_initializer(new Opm::GridPropertyConstantInitializer<DataType>(defaultValue)),
          m_postProcessor(postProcessor),
          m_dimensionString(dimString)
    {}

        

    GridPropertySupportedKeywordInfo(const GridPropertySupportedKeywordInfo&) = default;


    const std::string& getKeywordName() const {
        return m_keywordName;
    }


    const std::string& getDimensionString() const {
        return m_dimensionString;
    }


    std::shared_ptr<const Initializer> getInitializer() const {
        return m_initializer;
    }

    
    std::shared_ptr<const PostProcessor> getPostProcessor() const {
        return m_postProcessor;
    }

    
    bool hasPostProcessor() const {
        return static_cast<bool>(m_postProcessor);
    }   



private:
    std::string m_keywordName;
    std::shared_ptr<const Initializer> m_initializer;
    std::shared_ptr<const PostProcessor> m_postProcessor;
    std::string m_dimensionString;
};



template <typename T>
class GridProperty {
public:
    typedef GridPropertySupportedKeywordInfo<T> SupportedKeywordInfo;

    GridProperty(size_t nx , size_t ny , size_t nz , const SupportedKeywordInfo& kwInfo) {
        m_nx = nx;
        m_ny = ny;
        m_nz = nz;
        m_kwInfo = kwInfo;
        m_data.resize( nx * ny * nz );
        
        m_kwInfo.getInitializer()->apply(m_data, m_kwInfo.getKeywordName());
        m_hasRunPostProcessor = false;
    }

    size_t getCartesianSize() const {
        return m_data.size();
    }

    size_t getNX() const {
        return m_nx;
    }

    size_t getNY() const {
        return m_ny;
    }

    size_t getNZ() const {
        return m_nz;
    }
    
    
    T iget(size_t index) const {
        if (index < m_data.size()) {
            return m_data[index];
        } else {
            throw std::invalid_argument("Index out of range \n");
        }
    }


    T iget(size_t i , size_t j , size_t k) const {
        size_t g = i + j*m_nx + k*m_nx*m_ny;
        return iget(g);
    }

    void iset(size_t index, T value) {
        if (index < m_data.size())
            m_data[index] = value;
        else 
            throw std::invalid_argument("Index out of range \n");
    }

    void iset(size_t i , size_t j , size_t k , T value) {
        size_t g = i + j*m_nx + k*m_nx*m_ny;
        iset(g,value);
    }

    bool containsNaN( ) {
        throw std::logic_error("Only <double> and can be meaningfully queried for nan");
    }

    void multiplyWith(const GridProperty<T>& other) {
        if ((m_nx == other.m_nx) && (m_ny == other.m_ny) && (m_nz == other.m_nz)) {
            for (size_t g=0; g < m_data.size(); g++)
                m_data[g] *= other.m_data[g];
        } else
            throw std::invalid_argument("Size mismatch between properties in mulitplyWith.");
    }


    void multiplyValueAtIndex(size_t index, T factor) {
        m_data[index] *= factor;
    }

    const std::vector<T>& getData() const {
        return m_data;
    }

    void loadFromDeckKeyword(std::shared_ptr<const Box> inputBox, DeckKeywordConstPtr deckKeyword) {
        const auto deckItem = getDeckItem(deckKeyword);

        const std::vector<size_t>& indexList = inputBox->getIndexList();
        for (size_t sourceIdx = 0; sourceIdx < indexList.size(); sourceIdx++) {
            size_t targetIdx = indexList[sourceIdx];
            if (sourceIdx < deckItem->size()
                && !deckItem->defaultApplied(sourceIdx))
            {
                setDataPoint(sourceIdx, targetIdx, deckItem);
            }
        }
    }

    void loadFromDeckKeyword(DeckKeywordConstPtr deckKeyword) {
        const auto deckItem = getDeckItem(deckKeyword);

        for (size_t dataPointIdx = 0; dataPointIdx < deckItem->size(); ++dataPointIdx) {
            if (!deckItem->defaultApplied(dataPointIdx))
                setDataPoint(dataPointIdx, dataPointIdx, deckItem);
        }
    }

    void copyFrom(const GridProperty<T>& src, std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            for (size_t i = 0; i < src.getCartesianSize(); ++i)
                m_data[i] = src.m_data[i];
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] = src.m_data[targetIndex];
            }
        }
    }
    
    void scale(T scaleFactor , std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            for (size_t i = 0; i < m_data.size(); ++i)
                m_data[i] *= scaleFactor;
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] *= scaleFactor;
            }
        }
    }


    void add(T shiftValue , std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            for (size_t i = 0; i < m_data.size(); ++i)
                m_data[i] += shiftValue;
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] += shiftValue;
            }
        }
    }


    

    void setScalar(T value , std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            std::fill(m_data.begin(), m_data.end(), value);
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] = value;
            }
        }
    }

    const std::string& getKeywordName() const {
        return m_kwInfo.getKeywordName();
    }

    const SupportedKeywordInfo& getKeywordInfo() const {
        return m_kwInfo;
    }

    
    bool postProcessorRunRequired() {
        if (m_kwInfo.hasPostProcessor() && !m_hasRunPostProcessor)
            return true;
        else
            return false;
    }


    void runPostProcessor() {
        if (postProcessorRunRequired()) {
            // This is set here before the post processor has actually
            // completed; this is to protect against circular loops if
            // the post processor itself calls for the same grid
            // property.
            m_hasRunPostProcessor = true;
            auto postProcessor = m_kwInfo.getPostProcessor();
            postProcessor->apply( m_data );
        }
    }
    
    


private:
    Opm::DeckItemConstPtr getDeckItem(Opm::DeckKeywordConstPtr deckKeyword) {
        if (deckKeyword->size() != 1)
            throw std::invalid_argument("Grid properties can only have a single record (keyword "
                                        + deckKeyword->name() + ")");
        if (deckKeyword->getRecord(0)->size() != 1)
            // this is an error of the definition of the ParserKeyword (most likely in
            // the corresponding JSON file)
            throw std::invalid_argument("Grid properties may only exhibit a single item  (keyword "
                                        + deckKeyword->name() + ")");

        const auto deckItem = deckKeyword->getRecord(0)->getItem(0);

        if (deckItem->size() > m_data.size())
            throw std::invalid_argument("Size mismatch when setting data for:" + getKeywordName() +
                                        " keyword size: " + boost::lexical_cast<std::string>(deckItem->size())
                                        + " input size: " + boost::lexical_cast<std::string>(m_data.size()));

        return deckItem;
    }

    void setDataPoint(size_t sourceIdx, size_t targetIdx, Opm::DeckItemConstPtr deckItem);

    size_t      m_nx,m_ny,m_nz;
    SupportedKeywordInfo m_kwInfo;
    std::vector<T> m_data;
    bool m_hasRunPostProcessor;
};

}
#endif
