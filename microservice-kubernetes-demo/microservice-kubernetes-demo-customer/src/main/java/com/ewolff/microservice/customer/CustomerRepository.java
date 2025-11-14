package com.ewolff.microservice.customer;

import java.util.List;

import org.springframework.data.mongodb.repository.MongoRepository;
import org.springframework.data.repository.query.Param;
import org.springframework.data.rest.core.annotation.RepositoryRestResource;

@RepositoryRestResource(collectionResourceRel = "customer", path = "customer")
public interface CustomerRepository extends
		MongoRepository<Customer, String> {

	List<Customer> findByName(@Param("name") String name);

}
