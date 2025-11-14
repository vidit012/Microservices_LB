package com.ewolff.microservice.order.logic;

import org.springframework.data.repository.PagingAndSortingRepository;

public interface SimpleOrderRepository extends PagingAndSortingRepository<SimpleOrder, Long> {
	// Spring Data JPA will automatically implement basic CRUD operations
}
